#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <vector>

#include "fixed_string.h"

namespace pformat {

namespace internal {

struct state_in {};
struct state_out {};
struct state_out_escape {};

enum class format_type { PARAMETER, ELEMENT };

template <const auto &grammer_str, size_t start_v, size_t end_v>
struct format_element {
    constexpr static size_t start = start_v;
    constexpr static size_t end = end_v;

    constexpr static const char *c = grammer_str.c_str() + start;

    constexpr static size_t s = end - start;
    static constexpr auto type = format_type::ELEMENT;

    static constexpr size_t size() noexcept { return s; }

    static constexpr const char *c_str() noexcept { return c; }
};

struct format_parameter {
    static constexpr auto type = format_type::PARAMETER;
};

/**
 * Prototype for a format visitor
 * for visit
 */
struct format_visitor {
    template <typename element_t>
    void visit_element(element_t const &) {}

    template <typename parameter_t>
    void visit_parameter(parameter_t const &) {}
};

/**
 * Prototype for a format visitor
 * for visit_with_tuple
 */
struct format_visitor_with_tuple {
    template <typename element_t>
    void visit_element(element_t const &) {}

    template <typename parameter_t, typename arg_t>
    void visit_parameter(parameter_t const &, arg_t const &) {}
};

// result of a format parsing
template <bool ok, int parameter_count, typename... element_t>
struct format_result {
    static constexpr auto is_valid_format_string() noexcept { return ok; }

    static constexpr auto get_parameter_count() noexcept {
        return parameter_count;
    }

    // the visit function using the visitor pattern
    // is the main way inspect the format result.
    template <typename visitor_t>
    inline static constexpr void visit(visitor_t &v) {
        static_assert(ok);
        return visit_helper<visitor_t, element_t...>(v);
    }

    // combines the format result with the arguments (for format parameter)
    template <typename visitor_t, typename tuple_t>
    inline static constexpr void visit_with_tuple(visitor_t &v, tuple_t &&t) {
        static_assert(ok);
        return visit_with_tuple_helper<visitor_t, 0, decltype(t), element_t...>(
            v, t);
    }

   private:
    template <typename visitor_t, typename first_t, typename... rest_t>
    inline static constexpr void visit_helper(visitor_t &v) {
        if constexpr (first_t::type == format_type::PARAMETER) {
            v.visit_parameter(first_t());
        } else {
            v.visit_element(first_t());
        }
        visit_helper<visitor_t, rest_t...>(v);
    }

    template <typename visitor_t>
    inline static constexpr void visit_helper(visitor_t &) {
        return;
    }

    template <typename visitor_t, int n, typename tuple_t, typename first_t,
              typename... rest_t>
    inline static constexpr void visit_with_tuple_helper(visitor_t &v,
                                                         tuple_t const &t) {
        if constexpr (first_t::type == format_type::PARAMETER) {
            v.visit_parameter(first_t(), std::get<n>(t));
            visit_with_tuple_helper<visitor_t, n + 1, tuple_t, rest_t...>(v, t);
        } else {
            v.visit_element(first_t());
            visit_with_tuple_helper<visitor_t, n, tuple_t, rest_t...>(v, t);
        }
    }

    template <typename visitor_t, int n, typename tuple_t>
    inline static constexpr void visit_with_tuple_helper(visitor_t &,
                                                         tuple_t const &) {
        return;
    }
};

template <char... charpack>
struct grammer_str {
    constexpr static auto str =
        fixed_string<sizeof...(charpack) + 1>({charpack...});
};

// grammer for parsing
//
// grammer_str: reference to a grammer_str instance
// n: current offset in the grammer string
// start_stack: start of next "format_element"
// parameter_count: number of format_parameter elements
// result_t: format_element/format_parameter types, which is
// form the mainresult of the grammer 'execution'.
template <const auto &grammer_str, int n, int start_stack, int parameter_count,
          typename... result_t>
struct grammer {
    static constexpr auto next(state_in) {
        constexpr auto c = grammer_str[n];
        if constexpr (c == '}') {
            return grammer<grammer_str, n + 1, n + 1, parameter_count + 1,
                           result_t..., format_parameter>::next(state_out());
        } else if constexpr (c == '{') {
            // {{ escape
            return grammer<grammer_str, n + 1, n, parameter_count,
                           result_t...>::next(state_out());
        } else {
            return format_result<false, 0>();
        }
    }

    static constexpr auto next(state_out) {
        constexpr auto c = grammer_str[n];
        if constexpr (c == '{' && start_stack != n) {
            using next_element_t = format_element<grammer_str, start_stack, n>;
            return grammer<grammer_str, n + 1, n, parameter_count, result_t...,
                           next_element_t>::next(state_in());
        } else if constexpr (c == '{') {
            return grammer<grammer_str, n + 1, n, parameter_count,
                           result_t...>::next(state_in());
        } else if constexpr (c == '}') {
            return grammer<grammer_str, n + 1, start_stack, parameter_count,
                           result_t...>::next(state_out_escape());
        } else if constexpr (c == 0 && start_stack != n) {
            using next_element_t = format_element<grammer_str, start_stack, n>;
            return format_result<true, parameter_count, result_t...,
                                 next_element_t>();
        } else if constexpr (c == 0) {
            return format_result<true, parameter_count, result_t...>();
        } else {
            return grammer<grammer_str, n + 1, start_stack, parameter_count,
                           result_t...>::next(state_out());
        }
    }

    static constexpr auto next(state_out_escape) {
        constexpr auto c = grammer_str[n];
        if constexpr (c == '}') {
            // closed the escape }}
            using next_element_t = format_element<grammer_str, start_stack, n>;
            return grammer<grammer_str, n + 1, n + 1, parameter_count,
                           result_t..., next_element_t>::next(state_out());
        } else {
            return format_result<false, 0>();
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
    return grammer<grammer_str<charpack...>::str, 0, 0, 0>::next(state_out());
}

}  // namespace internal

}  // namespace pformat