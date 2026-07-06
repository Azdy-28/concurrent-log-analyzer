#include <cstdlib>
#include <iostream>
using namespace std;
#include "log_stats.hpp"

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__  \
                       << "\n";                                                        \
            exit(1);                                                             \
        }                                                                              \
    } while (0)

int main() {
    {
        LogStats stats;
        tokenize_line("2026-07-18T10:15:23.512Z ERROR auth: token validation failed", stats);
        CHECK(stats.total_lines == 1);
        CHECK(stats.malformed_lines == 0);
        CHECK(stats.level_counts[static_cast<size_t>(LogLevel::ERROR)] == 1);
    }

    {
        LogStats stats;
        tokenize_line("ts TRACE m: x", stats);
        tokenize_line("ts DEBUG m: x", stats);
        tokenize_line("ts INFO m: x", stats);
        tokenize_line("ts WARN m: x", stats);
        tokenize_line("ts ERROR m: x", stats);
        tokenize_line("ts FATAL m: x", stats);
        CHECK(stats.level_counts[static_cast<size_t>(LogLevel::TRACE)] == 1);
        CHECK(stats.level_counts[static_cast<size_t>(LogLevel::DEBUG)] == 1);
        CHECK(stats.level_counts[static_cast<size_t>(LogLevel::INFO)] == 1);
        CHECK(stats.level_counts[static_cast<size_t>(LogLevel::WARN)] == 1);
        CHECK(stats.level_counts[static_cast<size_t>(LogLevel::ERROR)] == 1);
        CHECK(stats.level_counts[static_cast<size_t>(LogLevel::FATAL)] == 1);
        CHECK(stats.total_lines == 6);
    }

    {
        LogStats stats;
        tokenize_line("ts WEIRD m: x", stats);
        CHECK(stats.level_counts[static_cast<size_t>(LogLevel::UNKNOWN)] == 1);
        CHECK(stats.malformed_lines == 0); 
    }

    {
        LogStats stats;
        tokenize_line("nospaceshere", stats);
        CHECK(stats.malformed_lines == 1);
    }

    {
        LogStats stats;
        tokenize_line("ts INFO", stats);
        CHECK(stats.level_counts[static_cast<size_t>(LogLevel::INFO)] == 1);
        CHECK(stats.malformed_lines == 0);
    }

    {
        LogStats stats;
        tokenize_line("", stats);
        CHECK(stats.total_lines == 1);
        CHECK(stats.malformed_lines == 1);
    }

    {
        LogStats a, b;
        tokenize_line("ts ERROR m: x", a);
        tokenize_line("ts ERROR m: x", b);
        tokenize_line("ts INFO m: x", b);
        a.merge(b);
        CHECK(a.total_lines == 3);
        CHECK(a.level_counts[static_cast<size_t>(LogLevel::ERROR)] == 2);
        CHECK(a.level_counts[static_cast<size_t>(LogLevel::INFO)] == 1);
    }

    cout << "All tokenizer tests passed.\n";
    return 0;
}
