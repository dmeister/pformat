#pragma once

#include <cstdint>
#include <utility>

#include "fixed_string.h"

namespace pformat {

namespace internal {

// format element copying a substring of the grammer to the output
struct format_element {
    std::size_t start{};
    std::size_t end{};
    bool set{};

    constexpr bool empty() const noexcept {
        return start == end;
    }

    constexpr std::size_t size() const noexcept { return end - start; }
};

enum class grammer_state { out, in, escape, invalid };

// result of a format parsing
template <int n, size_t str_size>
struct format_result {
    std::size_t current_str{};
    char str[str_size+1]{};

    std::size_t current_result{0};
    format_element results[n]{};
    bool valid{true};

    constexpr void clear() { valid=false; }

    constexpr void push_element(char const * const input_str, std::size_t start, std::size_t end) {
        auto & e = results[current_result];
        if (!e.set) {
            e.start = current_str;
            e.set = true;
        }
        size_t i{};
        for (; i < end - start; i++) {
            str[current_str++] = input_str[start + i];
        }
        e.end = current_str;
        str[current_str] = 0;
    }

    constexpr void push_parameter() {
        current_result++;
    }

    // returns true iff this is a valid formatting string
    constexpr bool is_valid_format_string() const noexcept {
        return valid;
    }

    // returns the number of parameters
    constexpr auto get_parameter_count() const noexcept { return current_result; }

    // the visit function using the visitor pattern
    // is the main way inspect the format result.
    template <typename element_func_t, typename parameter_func_t,
              typename... args_t>
    inline constexpr void visit(element_func_t &&ev, parameter_func_t &&pv,
                                args_t &&... args) const {
        std::size_t i = 0;
        auto visit_helper = [this, &i, &ev, &pv](auto &&arg) {
            if (!results[i].empty()) {
                ev(str + results[i].start, results[i].size());
            }
            i++;
            pv(std::forward<decltype(arg)>(arg));
        };
        (visit_helper(args), ...);
        if (!results[i].empty()) {
            ev(str + results[i].start, results[i].size());
        }
    }

    constexpr void parse(char const * const str) {
        std::size_t start = 0;
        grammer_state state = grammer_state::out;

        std::size_t i = 0;
        for (; str[i] && state != grammer_state::invalid; i++) {
            auto c = str[i];
            if (state == grammer_state::out) {
                if (c == '{') {
                    push_element(str, start, i);
                    state = grammer_state::in;
                } else if (c == '}') {
                    push_element(str, start, i);
                    state = grammer_state::escape;
                }
            } else if (state == grammer_state::in) {
                if (c == '}') {
                    push_parameter();
                    state = grammer_state::out;
                    start = i + 1;
                } else if (c == '{') {
                    state = grammer_state::out;
                    start = i;
                } else {
                    state = grammer_state::invalid;
                }
            } else if (state == grammer_state::escape) {
                if (c == '}') {
                    start = i;
                    state = grammer_state::out;
                } else {
                    state = grammer_state::invalid;
                }
            }
        }
        if (state != grammer_state::out) {
            clear();
        } else {
            push_element(str, start, i);
        }
    }
};

static constexpr std::size_t count_formats(const char *str) {
    int count_open{1};
    int count_closing{1};
    for (size_t i = 0; str[i]; i++) {
        auto c = str[i];
        if (c == '{') {
            count_open++;
        } else if (c == '}') {
            count_closing++;
        }
    }
    if (count_open > count_closing) {
        return count_open;
    }
    return count_closing;
}

// parses the charpack
//
// returns a matching configured log_config
// check is_valid_format_string() to see if
// the parsing was successful or not.
template <char ... charpack>
constexpr auto parse_format() {
    constexpr fixed_string<sizeof...(charpack) + 1> str({charpack...});
    constexpr size_t str_size = sizeof...(charpack);
    constexpr auto format_count = count_formats(str.c_str());

    if constexpr (format_count == 0) {
        return format_result<0, 0>();
    } else {
        // so that there is re-used.
        constexpr size_t adjusted_str_size = std::max<size_t>(2, (str_size >> 1) << 2);
        using format_result_t = format_result<format_count, adjusted_str_size>;
        format_result_t result;
        result.parse(str.c_str());
        return result;
    }
}

}  // namespace internal

}  // namespace pformat