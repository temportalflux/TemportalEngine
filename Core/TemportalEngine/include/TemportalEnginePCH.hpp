#pragma once

#include "Optick/src/optick.h"

#include "Api.h"
#include "Namespace.hpp"
#include "types/integer.h"
#include "types/real.h"
#include "math/values.hpp"
#include "math/Vector.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <string>
#include <vector>

struct IUnknown;

#define FORWARD_DEF(ns_macro, fwd) ns_macro fwd; NS_END
