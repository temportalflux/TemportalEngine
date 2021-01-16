#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

#include "cereal/GraphicsFlags.hpp"
#include "cereal/list.hpp"
#include "cereal/mathVector.hpp"
#include "cereal/optional.hpp"

namespace cereal
{
	extern cereal::JSONOutputArchive::Options JsonFormat;
}
