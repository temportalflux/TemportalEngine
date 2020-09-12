#include "world/BlockInstanceMap.hpp"

using namespace world;

void BlockInstanceMap::updateCoordinate(
	world::Coordinate const &global,
	std::optional<BlockMetadata> const& prev,
	std::optional<BlockMetadata> const& next
)
{
	// TODO: render data needs to account for what chunk the player is currently in for it to render with the proper matrix
}
