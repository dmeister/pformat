#include <string>
#include <cstring>
#include <type_traits>
#include <algorithm>
#include <charconv>

namespace pformat {

namespace placement {

// base types for diag extensions
// for user defined types
struct format_extention {
    // return an upper bound on the number of characters unsafe_place
    // might produce
    virtual std::size_t placement_size() const = 0;

    // place the characters into the buffer and
    // return a pointer to the next element of the buffer.
    //
    // the caller has to ensure that placement_size() many
    // chars are available to use.
    //
    // unsafe_place is only allowed to return std::errc::value_too_large and std::errc();
    virtual std::to_chars_result unsafe_place(char *buf, char * buf_end) const = 0;
};

// basic reverse based itoa.
// can be optimized later
template <typename int_t,
          typename = std::enable_if_t<std::is_integral_v<int_t>>>
std::to_chars_result unsafe_place(char *buf, char * buf_end, int_t value) noexcept {

    return std::to_chars(buf, buf_end, value);
}

inline std::to_chars_result unsafe_place(char *buf, char * buf_end, char value) noexcept {
    if (buf == buf_end) {
        return {buf, std::errc::value_too_large};

    }
    *buf = value;
    return {buf + 1, std::errc()};
}

template <typename enum_t, std::enable_if_t<
                               std::is_enum_v<enum_t>, int> = 0>
std::to_chars_result unsafe_place(char *buf, char * buf_end, enum_t value) noexcept {
    using int_t = typename std::underlying_type<enum_t>::type;
    return unsafe_place(buf, buf_end, static_cast<int_t>(value));
}

inline std::to_chars_result unsafe_place(char *buf, char *, double value) noexcept {
    // strangely to_chars appears to not be implemented
    auto i = std::snprintf(buf, 20, "%f", value);
    return {buf + i, std::errc()};
}

inline std::to_chars_result unsafe_place(char *buf, char * buf_end, bool value) noexcept {
    int l = 4 + !value;
    if (buf + l >= buf_end) {
            return {buf, std::errc::value_too_large};
    }
    std::memcpy(buf, value ? "true" : "false", l);
    return {buf + l, std::errc()};
}

inline std::to_chars_result unsafe_place(char *buf, char *, std::string const &value) noexcept {
    int l = value.size();
    std::memcpy(buf, value.data(), l);
    return {buf + l, std::errc()};
}

inline std::to_chars_result unsafe_place(char *buf, char *,std::string_view const &value) noexcept {
    int l = value.size();
    std::memcpy(buf, value.data(), l);
    return {buf + l, std::errc()};
}

inline std::to_chars_result unsafe_place(char *buf, char*,char const *s) noexcept {
    auto l = strlen(s);
    std::memcpy(buf, s, l);
    return {buf + l, std::errc()};
}

inline std::to_chars_result unsafe_place(char *buf, char *,char const *str_begin,
                          std::size_t len) noexcept {
    std::memcpy(buf, str_begin, len);
    return {buf + len, std::errc()};
}

template <typename format_extention_t,
          std::enable_if_t<std::is_base_of_v<
              format_extention, format_extention_t>, int> = 0>
inline std::to_chars_result unsafe_place(char *buf, char * buf_end, format_extention_t const &d) {
    return d.unsafe_place(buf, buf_end);
}

// placement_size returns the maximal size a value can take in a formatted
// string.
//
// we assume the longest integral is 64-bit and use that number
template <typename int_t, std::enable_if_t<
                              std::is_integral_v<int_t>, int> = 0>
constexpr std::size_t placement_size(int_t) noexcept {
    return std::numeric_limits<int_t>::digits10;
}

// size of a floating point number if placed
template <typename float_t,
          std::enable_if_t<std::is_floating_point_v<float_t>, int> = 0>
constexpr std::size_t placement_size(float_t) noexcept {
    return std::numeric_limits<float_t>::digits10;
}

// size of a char if placed
constexpr std::size_t placement_size(char) noexcept { return 1; }


// size of a bool if placed
constexpr std::size_t placement_size(bool) noexcept { return 5; }

// size of an enum if placed
//
// We use the underlying type
template <typename enum_t, std::enable_if_t<
                               std::is_enum_v<enum_t>, int> = 0>
constexpr std::size_t placement_size(enum_t v) noexcept {
    using int_t = typename std::underlying_type<enum_t>::type;
    return placement_size(static_cast<int_t>(v));
}

// size of a a format extension when placed
template <typename format_extention_t,
          std::enable_if_t<std::is_base_of_v<
              format_extention, format_extention_t>, int> = 0>
constexpr std::size_t placement_size(format_extention_t const &d) noexcept {
    return d.placement_size();
}

// size of a std::string if placed
inline std::size_t placement_size(std::string const &s) noexcept { return s.size(); }

// size of a string_view if placed
inline constexpr std::size_t placement_size(std::string_view const &s) noexcept {
    return s.size();
}

// size of a const char * if placed
inline std::size_t placement_size(const char *s) noexcept { return std::strlen(s); }

namespace internal {
// helper to check if a type can be placed.
// In particular, it checks the availability of
// placement_size and unsafe_place for the type.
template <typename = void, typename... Args>
struct placement_size_test : std::false_type {};

template <typename... Args>
struct placement_size_test<
    std::void_t<decltype(placement_size(std::declval<Args>()...))>, Args...>
    : std::true_type {};

template <typename = void, typename... Args>
struct unsafe_place_test : std::false_type {};

template <typename... Args>
struct unsafe_place_test<std::void_t<decltype(unsafe_place(
                             std::declval<char *>(), std::declval<char *>(), std::declval<Args>()...))>,
                         Args...> : std::true_type {};

template <typename type_t>
constexpr bool is_placeable() noexcept {
    return placement_size_test<void, type_t>::value &&
           unsafe_place_test<void, type_t>::value;
}

}  // namespace internal

template <typename... type_t>
constexpr bool test_placements() {
    return (internal::is_placeable<type_t>() && ...);
}

};  // namespace placement

template <typename pointer_t>
struct pointer_format_extention final : placement::format_extention {
    const pointer_t p;

    constexpr pointer_format_extention(pointer_t p_) : p(p_) {}

    inline std::size_t placement_size() const override {
        return std::numeric_limits<std::size_t>::digits10 + 2;
    }

    inline std::to_chars_result unsafe_place(char *buf, char * buf_end) const override {
        if (buf_end - buf < static_cast<int64_t>(placement_size())) {
            return {buf, std::errc::value_too_large};
        }
        buf[0] = '0';
        buf[1] = 'x';
        const std::size_t v = reinterpret_cast<std::size_t>(p);
        return std::to_chars(buf + 2, buf_end, v, 16);
    }
};

template <typename value_t,
    typename pointer_t = typename std::add_pointer<value_t>::type,
    typename extension_t = pointer_format_extention<pointer_t>>
auto any(value_t *p) {
    return extension_t(p);
}

}  // namespace pformat