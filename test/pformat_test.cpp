#include <gtest/gtest.h>
#include <pformat/pformat.h>

#include <numeric>

TEST(PFormat, Format) {
    using namespace pformat;
    // this example shows the compiled format style of usage
    auto s = "foo {} bar {} do {}"_log.format(1, 2, "bar");

    ASSERT_EQ(s, "foo 1 bar 2 do bar");
}

TEST(PFormat, FormatWithCall) {
    using namespace pformat;
    // this example shows the compiled format style of usage
    auto s = "foo {} bar {} do {}"_log(1, 2, "bar");
    ASSERT_EQ(s, "foo 1 bar 2 do bar");
}

TEST(Pformat, EmptyLiteral) {
    using namespace pformat;

    constexpr auto f = ""_log;
    auto s = f.format();
    ASSERT_EQ(s.size(), 0U);
    ASSERT_EQ(s, "");
}

TEST(Pformat, SimpleLiterals) {
    using namespace pformat;

    constexpr auto f = "foo {} bar {} do {}"_log;
    ASSERT_EQ(f.string_size_bound("a", "", "c"), 16U);
    auto s = f.format("a", "", "c");
    ASSERT_EQ(s, "foo a bar  do c");
    ASSERT_LE(s.size(), 16U);

    constexpr auto f2 = "foo {} bar {} d"_log;
    s = f2.format("a", "");
    ASSERT_EQ(s, "foo a bar  d");
}

TEST(Pformat, FormatTo) {
    using namespace pformat;

    char buf[100];
    constexpr auto f = "foo {} bar {} do {}"_log;
    f.format_to(buf, buf + 10, "a", "", "c");
    ASSERT_EQ(strcmp(buf, "foo a bar  do c"), 0);
}

TEST(Pformat, Escaping) {
    using namespace pformat;

    constexpr auto f = "foo {{foo"_log;
    ASSERT_EQ(f.format(), "foo {foo");

    constexpr auto f2 = "foo {{{}"_log;
    ASSERT_EQ(f2.format("bar"), "foo {bar");

    constexpr auto f3 = "foo }}"_log;
    ASSERT_EQ(f3.format(), "foo }");

    constexpr auto f4 = "foo }}bar"_log;
    ASSERT_EQ(f4.format(), "foo }bar");

    constexpr auto f5 = "foo }}{}"_log;
    ASSERT_EQ(f5.format("bar"), "foo }bar");

    constexpr auto f6 = "foo {{}} bar"_log;
    ASSERT_EQ(f6.format(), "foo {} bar");
}

TEST(Pformat, FormatInteger) {
    using namespace pformat;

    constexpr auto f = "foo {} bar {} do {}"_log;
    uint64_t a = std::numeric_limits<uint64_t>::max();
    int64_t b = -17;
    int64_t c = std::numeric_limits<int64_t>::min();
    auto s = f.format(a, b, c);
    ASSERT_EQ(s, "foo 18446744073709551615 bar -17 do -9223372036854775808");
}

TEST(Pformat, FormatDouble) {
    using namespace pformat;

    constexpr auto f = "foo {} bar {} do {}"_log;
    double a = 0.0;
    double b = std::numeric_limits<double>::infinity();
    double c = std::numeric_limits<double>::quiet_NaN();
    auto s = f.format(a, b, c);
    ASSERT_EQ(s, "foo 0.000000 bar inf do nan");
}

TEST(Pformat, FormatChar) {
    using namespace pformat;

    constexpr auto f = "foo {} bar {}"_log;
    char a = 'a';
    unsigned char b = 0xff;
    auto s = f.format(a, b);
    ASSERT_EQ(s, "foo a bar 255");
}

TEST(Pformat, FormatBool) {
    using namespace pformat;

    constexpr auto f = "foo {} bar {}"_log;
    auto s = f.format(true, false);
    ASSERT_EQ(s, "foo true bar false");
}

TEST(Pformat, FormatString) {
    using namespace pformat;

    constexpr auto f = "x{}y"_log;
    std::string str{"foo"};
    ASSERT_EQ(f.format(str), "xfooy");

    ASSERT_EQ(f.format(std::move(str)), "xfooy");
}

TEST(Pformat, FormatStringView) {
    using namespace pformat;

    constexpr auto f = "x{}y"_log;
    std::string str{"foo"};
    std::string_view sv(str);
    ASSERT_EQ(f.format(sv), "xfooy");
}

namespace {
class adl_class {};

size_t placement_size(adl_class const &) { return 4; }

std::to_chars_result unsafe_place(char *buf, char *, adl_class const &) {
    buf[0] = 'a';
    buf[1] = 's';
    buf[2] = 'd';
    buf[3] = 'f';
    return {buf + 4, std::errc()};
}
}  // namespace
TEST(Pformat, FormatWithADL) {
    using namespace pformat;

    constexpr auto f = "x{}y"_log;
    adl_class c;
    ASSERT_EQ(f.format(c), "xasdfy");
}

TEST(Pformat, FormatPointer) {
    using namespace pformat;

    uint64_t v;
    constexpr auto f = "{}"_log;
    auto s = f.format(any(&v));

    char compare_buf[30];
    std::snprintf(compare_buf, 30, "%p", &v);
    ASSERT_EQ(s, compare_buf);
}

namespace {
enum some_enum { SOME_ENUM_A, SOME_ENUM_B };

struct some_class {};

}  // namespace

TEST(Pformat, FormatEnum) {
    using namespace pformat;

    constexpr auto f = "{}{}"_log;
    auto s = f.format(SOME_ENUM_A, SOME_ENUM_B);

    ASSERT_EQ(s, "01");
}

// COMPILE ERROR EXPECTED
// TEST(Pformat, CompileErrorExpectedNotPlaceable) {
//    using namespace pformat;

//    constexpr auto f = "{}"_log;
//    some_class c;
//    f.format(c);
//}

// TEST(Pformat, CompileErrorExceptedTooFewParameterCount) {
//    using namespace pformat;
//
//    constexpr auto f = "{}"_log;
//    f.format();
//}

// TEST(Pformat, CompileErrorExceptedTooManyParameterCount) {
//    using namespace pformat;
//
//    constexpr auto f = "{}"_log;
//    f.format(1, 2);
//}