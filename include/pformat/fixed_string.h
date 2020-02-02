#pragma once

#include <stddef.h>

namespace pformat {
namespace internal {

/**
 * very simplified constexpr fixed string
 * based on document P0259R0, P0732R2
 */
template <size_t N>
struct fixed_string {
    char str[N] = {};
    size_t internal_size = 0;

    constexpr fixed_string(const fixed_string& other) noexcept {
        for (size_t i{0}; i <= N; ++i) {
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

    constexpr size_t size() const noexcept { return internal_size; }
    constexpr const char* begin() const noexcept { return str; }
    constexpr const char* end() const noexcept { return str + size(); }
    constexpr char operator[](size_t i) const noexcept { return str[i]; }

    constexpr const char* c_str() const { return str; }
};

static_assert(fixed_string<1>("").size() == 0);
static_assert(fixed_string<3>("ab").size() == 2);
static_assert(fixed_string<3>("ab")[0] == 'a');
static_assert(fixed_string<3>("ab")[1] == 'b');
static_assert(fixed_string<3>("ab")[2] == 0);

}  // namespace internal
}  // namespace pformat