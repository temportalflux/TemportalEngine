#include "ecs/types.h"

#include "utility/StringUtils.hpp"

template <>
std::string utility::StringParser<ecs::EType>::to_string(ecs::EType const& v)
{
	switch (v)
	{
	case ecs::EType::eEntity: return "entity";
	case ecs::EType::eView: return "view";
	case ecs::EType::eComponent: return "component";
	case ecs::EType::eSystem: return "system";
	default: return "invalid";
	}
}
