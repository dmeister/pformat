#include <vector>

#include "pformat/placement.h"

using namespace pformat::placement::internal;

// static assert testing (not in header file to not incur cost when used)
static_assert(is_placeable<int>());
static_assert(is_placeable<char const *>());
static_assert(is_placeable<bool>());
static_assert(!is_placeable<std::vector<int>>());