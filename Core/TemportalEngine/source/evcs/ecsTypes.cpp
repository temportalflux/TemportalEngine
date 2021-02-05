#include "evcs/types.h"

#include "utility/StringUtils.hpp"

template <>
std::string utility::StringParser<evcs::EType>::to_string(evcs::EType const& v)
{
	switch (v)
	{
	case evcs::EType::eEntity: return "entity";
	case evcs::EType::eView: return "view";
	case evcs::EType::eComponent: return "component";
	case evcs::EType::eSystem: return "system";
	default: return "invalid";
	}
}
