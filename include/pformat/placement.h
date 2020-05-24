#include <string>
#include <type_traits>
#include <algorithm>

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
    virtual char *unsafe_place(char *buf) const = 0;
};

// basic reverse based itoa.
// can be optimized later
template <typename int_t,
          typename std::enable_if<std::is_integral<int_t>::value &&
                                      std::is_signed<int_t>::value,
                                  int>::type * = nullptr>
char *unsafe_place(char *buf, int_t value) noexcept {
    using unsigned_int_t = typename std::make_unsigned<int_t>::type;
    bool neg = value < int_t();
    unsigned_int_t i = unsigned_int_t() +
                       (-(neg * 2 - 1) * static_cast<unsigned_int_t>(value));
    char *p = buf;

    do {
        unsigned_int_t lsd = i % 10U;
        i /= 10;
        *p++ = 0x30 + lsd;  // 0x30 is 0 in ASCSII
    } while (i != 0);

    if (neg) {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);
    return p;
}

template <unsigned base_v>
inline char integer_place_char(char i) noexcept {
    static_assert(base_v == 10 || base_v == 16,
                  "Only 10 and 16 are supported as bases");
    constexpr char base = '0';
    if constexpr (base_v == 10) {
        return base + i;
    }
    constexpr char diff = 'a' - (base + 10);
    bool higher =
        i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 15;
    auto offset = higher * diff;
    return base + offset + i;
}

template <
    typename int_t, unsigned base_v = 10,
    typename std::enable_if<std::is_integral<int_t>::value &&
                            !std::is_signed<int_t>::value>::type * = nullptr>
char *unsafe_place(char *buf, int_t value) noexcept {
    static_assert(base_v == 10 || base_v == 16,
                  "Only 10 and 16 are supported as bases");
    int_t i = value;
    char *p = buf;

    do {
        char lsd = i % base_v;
        i /= base_v;
        *p++ = integer_place_char<base_v>(lsd);
    } while (i != 0);
    *p = '\0';
    std::reverse(buf, p);
    return p;
}

inline char *unsafe_place(char *buf, char value) noexcept {
    *buf = value;
    return buf + 1;
}

template <typename enum_t, typename std::enable_if<
                               std::is_enum<enum_t>::value>::type * = nullptr>
char *unsafe_place(char *buf, enum_t value) noexcept {
    using int_t = typename std::underlying_type<enum_t>::type;
    return unsafe_place(buf, static_cast<int_t>(value));
}

inline char *unsafe_place(char *buf, double value) noexcept {
    // yeah, I don't really want to write this.
    auto i = std::snprintf(buf, 20, "%f", value);
    return buf + i;
}

inline char *unsafe_place(char *buf, bool value) noexcept {
    int l = 4 + !value;
    std::memcpy(buf, value ? "true" : "false", l);
    return buf + l;
}

inline char *unsafe_place(char *buf, std::string const &value) noexcept {
    int l = value.size();
    std::memcpy(buf, value.data(), l);
    return buf + l;
}

inline char *unsafe_place(char *buf, std::string_view const &value) noexcept {
    int l = value.size();
    std::memcpy(buf, value.data(), l);
    return buf + l;
}

inline char *unsafe_place(char *buf, char const *s) noexcept {
    auto l = strlen(s);
    std::memcpy(buf, s, l);
    return buf + l;
}

inline char *unsafe_place(char *buf, char const *str_begin,
                          std::size_t len) noexcept {
    std::memcpy(buf, str_begin, len);
    return buf + len;
}

template <typename format_extention_t,
          typename std::enable_if<std::is_base_of<
              format_extention, format_extention_t>::value>::type * = nullptr>
inline char *unsafe_place(char *buf, format_extention_t const &d) {
    return d.unsafe_place(buf);
}

// placement_size returns the maximal size a value can take in a formatted
// string.
//
// we assume the longest integral is 64-bit and use that number
template <typename int_t, typename std::enable_if<
                              std::is_integral<int_t>::value>::type * = nullptr>
constexpr std::size_t placement_size(int_t) noexcept {
    static_assert(sizeof(int_t) < 64, "Only integers to 64-bit are supported");
    // len(18446744073709551615) == 20
    return 20;
}

// size of a floating point number if placed
template <typename float_t,
          typename std::enable_if<std::is_floating_point<float_t>::value>::type
              * = nullptr>
constexpr std::size_t placement_size(float_t) noexcept {
    return 20;
}

// size of a char if placed
constexpr std::size_t placement_size(char) noexcept { return 1; }

// size of an enu if placed
//
// We use the underlying type
template <typename enum_t, typename std::enable_if<
                               std::is_enum<enum_t>::value>::type * = nullptr>
constexpr std::size_t placement_size(enum_t v) noexcept {
    using int_t = typename std::underlying_type<enum_t>::type;
    return placement_size(static_cast<int_t>(v));
}

// size of a a format extension when placed
template <typename format_extention_t,
          typename std::enable_if<std::is_base_of<
              format_extention, format_extention_t>::value>::type * = nullptr>
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
                             std::declval<char *>(), std::declval<Args>()...))>,
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
        return placement::placement_size<std::size_t>({}) + 2;
    }

    inline char *unsafe_place(char *buf) const override {
        buf[0] = '0';
        buf[1] = 'x';
        const std::size_t v = reinterpret_cast<std::size_t>(p);
        return placement::unsafe_place<std::size_t, 16>(buf + 2, v);
    }
};

template <typename value_t>
auto any(value_t *p) {
    using pointer_t = typename std::add_pointer<value_t>::type;
    return pointer_format_extention<pointer_t>(p);
}

}  // namespace pformat