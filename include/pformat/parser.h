#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <vector>

#include "fixed_string.h"

namespace pformat {

namespace internal {

struct state_in {};
struct state_out {};

enum class format_type { PARAMETER, ELEMENT };

// format element copying a substring of the grammer to the output
template <size_t start_v, size_t end_v>
struct format_element {
    static constexpr auto type = format_type::ELEMENT;

    static constexpr size_t size() noexcept { return s; }

    // offset into the grammer string
    constexpr static size_t start = start_v;
    // end offset within the grammer string
    constexpr static size_t end = end_v;

   private:
    constexpr static size_t s = end - start;
};

// format based on a parameter
template <size_t i>
struct format_parameter {
    static constexpr auto type = format_type::PARAMETER;
    constexpr static size_t index = i;
};

// result of a format parsing
template <std::string_view const &grammer_str, typename... element_t>
struct format_result {
    // returns true iff this is a valid formatting string
    static constexpr bool is_valid_format_string() noexcept {
        return sizeof...(element_t) > 0;
    }

    // returns the number of parameters
    static constexpr auto get_parameter_count() noexcept {
        size_t count{};
        visit([](auto) {}, [&count](auto) { count++; });
        return count;
    }

    static std::string_view const &str() noexcept { return grammer_str; }

    // the visit function using the visitor pattern
    // is the main way inspect the format result.
    template <typename element_func_t, typename parameter_func_t>
    inline static constexpr void visit(element_func_t &&ev,
                                       parameter_func_t &&pv) {
        static_assert(is_valid_format_string());
        visit_helper<element_func_t, parameter_func_t, element_t...>(
            std::forward<element_func_t>(ev),
            std::forward<parameter_func_t>(pv));
    }

   private:
    template <typename ev_t, typename pv_t, typename first_t,
              typename... rest_t>
    inline static constexpr void visit_helper(ev_t &&ev, pv_t &&pv) {
        if constexpr (first_t::type == format_type::PARAMETER) {
            pv(first_t{});
        } else {
            ev(first_t{});
        }
        visit_helper<ev_t, pv_t, rest_t...>(std::forward<ev_t>(ev),
                                            std::forward<pv_t>(pv));
    }

    template <typename ev_t, typename pv_t>
    inline static constexpr void visit_helper(ev_t &&, pv_t &&) {
        return;
    }
};

// the static string instance holding the grammer
// neither the fixed_str nor the string_view can be changed

template <char... charpack>
struct grammer_str {
    constexpr static auto fixed_str =
        fixed_string<sizeof...(charpack) + 1>({charpack...});
    constexpr static std::string_view str{
        grammer_str<charpack...>::fixed_str.view()};
};

// grammer for parsing
//
// grammer_str: reference to a grammer_str instance
// n: current offset in the grammer string
// start_stack: start of next "format_element"
// pc: number of format_parameter elements
// result_t: format_element/format_parameter types, which is
// form the mainresult of the grammer 'execution'.
template <std::string_view const &grammer_str, int n, int start_stack, int pc,
          typename... result_t>
struct grammer {
    // calculate the next format_result type

    // current state: outside a format parameter
    static constexpr auto next(state_out) {
        constexpr auto c = grammer_str[n];

        if constexpr (c == '{') {
            using next_element_t = format_element<start_stack, n>;
            return grammer<grammer_str, n + 1, n, pc, result_t...,
                           next_element_t>::next(state_in());
        } else if constexpr (c == 0) {
            // end of string
            using next_element_t = format_element<start_stack, n>;
            return format_result<grammer_str, result_t..., next_element_t>();
        } else {
            return grammer<grammer_str, n + 1, start_stack, pc,
                           result_t...>::next(state_out());
        }
    }

    // current state: within a format parameter
    static constexpr auto next(state_in) {
        constexpr auto c = grammer_str[n];
        if constexpr (c == '}') {  // } -> add format_parameter, move out
            return grammer<grammer_str, n + 1, n + 1, pc + 1,
                result_t..., format_parameter<pc>>::next(state_out());
        } else {
            // invalid
            return format_result<grammer_str>();
        }
    }

};

// parses the charpack
//
// returns a matching configured log_config
// check is_valid_format_string() to see if
// the parsing was successful or not.
template <char... charpack>
constexpr auto parse_format() {
    if constexpr (sizeof...(charpack) == 0) {
        return format_result<grammer_str<charpack...>::str,
                             format_element<0, 0>>();
    } else {
        return grammer<grammer_str<charpack...>::str, 0, 0, 0>::next(
            state_out());
    }
}

}  // namespace internal

}  // namespace pformat