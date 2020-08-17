#pragma once

#include "CoreInclude.hpp"

#include "BlockId.hpp"

struct BlockMetadata
{
	BlockMetadata() = default;
	BlockMetadata(game::BlockId id) : id(id) {}
	game::BlockId id;
};
