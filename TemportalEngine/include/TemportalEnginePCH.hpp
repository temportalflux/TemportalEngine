#pragma once

#include "Api.h"
#include "Namespace.hpp"
#include "types/integer.h"
#include "types/real.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <functional>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <string>
#include <vector>

struct IUnknown;

#define FORWARD_DEF(ns_macro, fwd) ns_macro fwd; NS_END
