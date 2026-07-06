#pragma once
#include <cstddef>
#include <string_view>

struct LineSpan {
    const char* data = nullptr;
    size_t len = 0;

    [[nodiscard]] std::string_view view() const noexcept {
        return std::string_view(data, len);
    }
};

static_assert(std::is_trivially_copyable_v<LineSpan>,
              "LineSpan must stay POD for cache-friendly batch storage");
