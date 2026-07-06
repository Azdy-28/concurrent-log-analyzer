#pragma once
#include <array>
#include <cstdint>
#include <string_view>

enum class LogLevel : uint8_t { TRACE = 0, DEBUG, INFO, WARN, ERROR, FATAL, UNKNOWN, COUNT };

inline constexpr std::array<const char*, static_cast<size_t>(LogLevel::COUNT)> kLevelNames = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "UNKNOWN"};

struct LogStats {
    std::array<uint64_t, static_cast<size_t>(LogLevel::COUNT)> level_counts{};
    uint64_t total_lines = 0;
    uint64_t total_bytes = 0;
    uint64_t malformed_lines = 0;

    void merge(const LogStats& other) {
        for (size_t i = 0; i < level_counts.size(); ++i) level_counts[i] += other.level_counts[i];
        total_lines += other.total_lines;
        total_bytes += other.total_bytes;
        malformed_lines += other.malformed_lines;
    }
};

inline LogLevel parse_level(std::string_view s) noexcept {
    switch (s.size()) {
        case 4:
            if (s == "INFO") return LogLevel::INFO;
            if (s == "WARN") return LogLevel::WARN;
            break;
        case 5:
            if (s == "TRACE") return LogLevel::TRACE;
            if (s == "DEBUG") return LogLevel::DEBUG;
            if (s == "ERROR") return LogLevel::ERROR;
            if (s == "FATAL") return LogLevel::FATAL;
            break;
        default:
            break;
    }
    return LogLevel::UNKNOWN;
}

inline void tokenize_line(std::string_view line, LogStats& stats) noexcept {
    stats.total_lines += 1;
    stats.total_bytes += line.size();

    const size_t first_space = line.find(' ');
    if (first_space == std::string_view::npos) {
        stats.malformed_lines += 1;
        return;
    }
    std::string_view rest = line.substr(first_space + 1); // substr on a view: no copy, just new (ptr,len)

    const size_t second_space = rest.find(' ');
    std::string_view level_str = (second_space == std::string_view::npos) ? rest : rest.substr(0, second_space);

    const LogLevel level = parse_level(level_str);
    stats.level_counts[static_cast<size_t>(level)] += 1;
}
