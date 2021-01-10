#pragma once

#include "optick.h"

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

struct virtual_enable_shared_from_this_base :
	std::enable_shared_from_this<virtual_enable_shared_from_this_base>
{
	virtual ~virtual_enable_shared_from_this_base() {}
};

template<typename T>
struct virtual_enable_shared_from_this :
	virtual virtual_enable_shared_from_this_base
{
	std::shared_ptr<T> shared_from_this()
	{
		return std::dynamic_pointer_cast<T>(
			virtual_enable_shared_from_this_base::shared_from_this()
		);
	}
	std::weak_ptr<T> weak_from_this()
	{
		return shared_from_this();
	}
};
