#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "bounded_queue.hpp"
#include "line_span.hpp"
#include "log_stats.hpp"
#include "mapped_file.hpp"

using namespace std;
using LineBatch = vector<LineSpan>;

struct Config {
    string file_path;
    size_t num_producers = 2;
    size_t num_consumers = max(2u, thread::hardware_concurrency());
    size_t batch_size = 4096;     
    size_t queue_capacity = 64;    
};

static vector<pair<size_t, size_t>> compute_ranges(const char* data, size_t size, size_t num_ranges) {
    vector<pair<size_t, size_t>> ranges;
    if (num_ranges == 0) num_ranges = 1;
    if (size == 0) return ranges;

    const size_t approx = size / num_ranges;
    size_t start = 0;
    for (size_t i = 0; i < num_ranges && start < size; ++i) {
        size_t end = (i == num_ranges - 1) ? size : min(size, start + approx);
        while (end < size && data[end] != '\n') ++end;
        if (end < size) ++end; 
        ranges.emplace_back(start, end);
        start = end;
    }
    return ranges;
}

static void producer_worker(const char* base, size_t start, size_t end, BoundedQueue<LineBatch>& work_queue,
                             BoundedQueue<LineBatch>& free_list, size_t batch_size) {
    LineBatch batch;
    if (!free_list.pop(batch)) return; 
    batch.clear();

    size_t pos = start;
    while (pos < end) {
        const char* line_start = base + pos;
        const void* nl = memchr(line_start, '\n', end - pos);
        size_t line_len, advance;
        if (nl != nullptr) {
            line_len = static_cast<const char*>(nl) - line_start;
            advance = line_len + 1;
        } else {
            line_len = end - pos;
            advance = line_len;
        }
        if (line_len > 0 && line_start[line_len - 1] == '\r') --line_len; 

        if (line_len > 0) {
            batch.push_back(LineSpan{line_start, line_len});
            if (batch.size() == batch_size) {
                work_queue.push(move(batch));
                if (!free_list.pop(batch)) return;
                batch.clear();
            }
        }
        pos += advance;
    }

    if (!batch.empty()) {
        work_queue.push(move(batch));
    } else {
        free_list.push(move(batch)); 
    }
}

static void consumer_worker(BoundedQueue<LineBatch>& work_queue, BoundedQueue<LineBatch>& free_list, LogStats& out_stats) {
    LineBatch batch;
    while (work_queue.pop(batch)) {
        for (const LineSpan& span : batch) {
            tokenize_line(span.view(), out_stats);
        }
        batch.clear();         
        free_list.push(move(batch));
    }
}

static void print_usage(const char* prog) {
    cerr << "Usage: " << prog << " <log_file> [--producers N] [--consumers N] "
              << "[--batch-size N] [--queue-capacity N]\n";
}

int main(int argc, char** argv) {
    Config cfg;
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    cfg.file_path = argv[1];
    for (int i = 2; i < argc; ++i) {
        string arg = argv[i];
        auto next = [&](const char* flag) -> size_t {
            if (i + 1 >= argc) throw runtime_error(string("Missing value for ") + flag);
            return static_cast<size_t>(stoul(argv[++i]));
        };
        if (arg == "--producers") cfg.num_producers = next("--producers");
        else if (arg == "--consumers") cfg.num_consumers = next("--consumers");
        else if (arg == "--batch-size") cfg.batch_size = next("--batch-size");
        else if (arg == "--queue-capacity") cfg.queue_capacity = next("--queue-capacity");
        else { print_usage(argv[0]); return 1; }
    }

    try {
        MappedFile file(cfg.file_path);
        if (file.size() == 0) {
            cout << "File is empty: " << cfg.file_path << "\n";
            return 0;
        }

        auto ranges = compute_ranges(file.data(), file.size(), cfg.num_producers);
        cfg.num_producers = ranges.size();

        BoundedQueue<LineBatch> work_queue(cfg.queue_capacity);
        
        BoundedQueue<LineBatch> free_list(cfg.queue_capacity + cfg.num_producers + 4);
        {
            size_t pool_size = cfg.queue_capacity + cfg.num_producers + 4;
            for (size_t i = 0; i < pool_size; ++i) {
                LineBatch b;
                b.reserve(cfg.batch_size);
                free_list.push(move(b));
            }
        }

        vector<LogStats> consumer_stats(cfg.num_consumers);

        const auto t_start = chrono::steady_clock::now();

        vector<thread> consumers;
        consumers.reserve(cfg.num_consumers);
        for (size_t i = 0; i < cfg.num_consumers; ++i) {
            consumers.emplace_back(consumer_worker, ref(work_queue), ref(free_list), ref(consumer_stats[i]));
        }

        vector<thread> producers;
        producers.reserve(cfg.num_producers);
        for (auto& [start, end] : ranges) {
            producers.emplace_back(producer_worker, file.data(), start, end, ref(work_queue), ref(free_list),
                                    cfg.batch_size);
        }

        for (auto& t : producers) t.join();
        work_queue.close();
        for (auto& t : consumers) t.join();

        const auto t_end = chrono::steady_clock::now();
        const double elapsed_s = chrono::duration<double>(t_end - t_start).count();

        LogStats total;
        for (const auto& s : consumer_stats) total.merge(s);

        const double mb = static_cast<double>(total.total_bytes) / (1024.0 * 1024.0);
        const double throughput = elapsed_s > 0 ? mb / elapsed_s : 0.0;

        cout << fixed << setprecision(2);
        cout << "\n=== Log Parsing Engine Report ===\n";
        cout << "File:              " << cfg.file_path << "\n";
        cout << "Producers:         " << cfg.num_producers << "\n";
        cout << "Consumers:         " << cfg.num_consumers << "\n";
        cout << "Batch size:        " << cfg.batch_size << " lines\n";
        cout << "Queue capacity:    " << cfg.queue_capacity << " batches\n";
        cout << "-----------------------------------\n";
        cout << "Total lines:       " << total.total_lines << "\n";
        cout << "Total bytes:       " << mb << " MB\n";
        cout << "Malformed lines:   " << total.malformed_lines << "\n";
        cout << "Elapsed:           " << elapsed_s << " s\n";
        cout << "Throughput:        " << throughput << " MB/s\n";
        cout << "-----------------------------------\n";
        for (size_t i = 0; i < static_cast<size_t>(LogLevel::COUNT); ++i) {
            cout << setw(8) << kLevelNames[i] << ": " << total.level_counts[i] << "\n";
        }
        cout << "===================================\n";

    } catch (const exception& ex) {
        cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
