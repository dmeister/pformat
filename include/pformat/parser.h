#pragma once

#include <cstdint>
#include <utility>
#include <algorithm>

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
    std::array<char, str_size+1> str{};

    std::size_t current_result{0};
    std::array<format_element, n> results{};
    bool valid{true};

    constexpr void clear() { valid=false; }

    constexpr void push_element(
        std::string_view input_str) {
        auto & e = results[current_result];
        if (!e.set) {
            e.start = current_str;
            e.set = true;
        }
        for (auto c : input_str) {
            str[current_str++] = c;
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
    template <typename func_t,
              typename... args_t>
    inline constexpr void visit(func_t && func,
                                args_t &&... args) const {
        if constexpr (sizeof...(args) == 0) {
            func(std::string_view{str.data() + results.front().start, results.front().size()});
        } else {
            auto result = results.begin();
            auto visit_helper = [this, &result, &func](auto &&arg) {
                if (!result->empty()) {
                    func(std::string_view{str.data() + result->start, result->size()});
                }
                result++;
                func(std::forward<decltype(arg)>(arg));
            };
            (visit_helper(args), ...);
            if (!result->empty()) {
                func(std::string_view{str.data() + result->start, result->size()});
            }
        }
    }

    constexpr void parse(std::string_view str) {
        grammer_state state = grammer_state::out;

        auto i = std::begin(str);
        auto const end = std::end(str);
        auto start = i;
        for (; i != end && state != grammer_state::invalid; i++) {
            auto c = *i;
            if (state == grammer_state::out) {
                if (c == '{') {
                    push_element(std::string_view(start, std::distance(start, i)));
                    state = grammer_state::in;
                } else if (c == '}') {
                    push_element(std::string_view(start, std::distance(start, i)));
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
            push_element(std::string_view(start, std::distance(start, i)));
        }
    }
};

template <char ... charpack>
static constexpr std::size_t count_formats() {
    std::initializer_list<char> str{charpack...};
    int count_open{1};
    int count_closing{1};
    for (auto c : str) {
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
template <char ... charpack,
    size_t format_count = count_formats<charpack...>(),
    size_t str_size = sizeof...(charpack),
    size_t adjusted_str_size = std::max<size_t>(2, (str_size >> 1) << 2),
    typename format_result_t = format_result<format_count, adjusted_str_size>>
constexpr auto parse_format() {

    if constexpr (format_count == 0) {
        return format_result<0, 0>();
    } else {
        constexpr fixed_string<sizeof...(charpack) + 1> str({charpack...});
        // so that there is re-used.
        format_result_t result;
        result.parse({str.c_str(), str.size()});
        return result;
    }
}

}  // namespace internal

}  // namespace pformat