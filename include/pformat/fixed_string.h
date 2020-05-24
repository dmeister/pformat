#pragma once

#include <cstddef>

namespace pformat {
namespace internal {

/**
 * very simplified constexpr fixed string
 * based on document P0259R0, P0732R2
 */
template <std::size_t N>
struct fixed_string {
    char str[N] = {};
    std::size_t internal_size = 0;

    constexpr fixed_string(const fixed_string& other) noexcept {
        for (std::size_t i{0}; i <= N; ++i) {
            str[i] = other.str[i];
        }
        internal_size = other.internal_size;
    }

    constexpr fixed_string(const char (&input)[N]) noexcept {
        for (size_t i = 0; i < N; ++i) {
            str[i] = input[i];
            if (input[i] == 0) {
                break;
            }
            internal_size++;
        }
    }

    constexpr std::size_t size() const noexcept { return internal_size; }
    constexpr const char* begin() const noexcept { return str; }
    constexpr const char* end() const noexcept { return str + size(); }
    constexpr char operator[](size_t i) const noexcept { return str[i]; }

    constexpr const char* c_str() const { return str; }
};

}  // namespace internal
}  // namespace pformat