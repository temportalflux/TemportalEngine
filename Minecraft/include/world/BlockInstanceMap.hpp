#pragma once

#include "CoreInclude.hpp"

#include "math/Matrix.hpp"

#include "world/BlockMetadata.hpp"
#include "world/WorldCoordinate.hpp"

NS_WORLD

class BlockInstanceMap
{

public:
	struct RenderData
	{
		math::Matrix4x4 model;
	};

	void updateCoordinate(
		world::Coordinate const &global,
		std::optional<BlockMetadata> const& prev,
		std::optional<BlockMetadata> const& next
	);

private:

};

NS_END
