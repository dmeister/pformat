#include "pformat/fixed_string.h"

using namespace pformat::internal;

// static assert testing (not in header file to not incur cost when used)
static_assert(fixed_string<1>("").size() == 0);
static_assert(fixed_string<3>("ab").size() == 2);
static_assert(fixed_string<3>("ab")[0] == 'a');
static_assert(fixed_string<3>("ab")[1] == 'b');
static_assert(fixed_string<3>("ab")[2] == 0);