#include <gtest/gtest.h>
#include "pformat/pformat.h"

// static assert testing (not in header file to not incur cost when used
using namespace pformat::internal;


static_assert(""_unchecked_log.ok());
static_assert("foo"_unchecked_log.ok());
static_assert("foo {}"_unchecked_log.ok());
static_assert(!"foo {a}"_unchecked_log.ok());
static_assert("foo {} bar {}"_unchecked_log.ok());
static_assert("foo {{"_unchecked_log.ok());
static_assert("foo }}"_unchecked_log.ok());
static_assert("foo {{}}"_unchecked_log.ok());
static_assert("foo }}"_unchecked_log.ok());
static_assert(!"foo {"_unchecked_log.ok());

TEST(PFormat, Parse) {
    constexpr auto s1 = ""_unchecked_log;
    ASSERT_TRUE(s1.ok());

    constexpr auto s2 = "foo {} bar {}"_unchecked_log;
    ASSERT_TRUE(s2.ok());

    constexpr auto s3 = "foo {a}"_unchecked_log;
    ASSERT_FALSE(s3.ok());
}