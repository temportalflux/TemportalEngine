#pragma once

#include "CoreInclude.hpp"

FORWARD_DEF(NS_GRAPHICS, class MinecraftRenderer);
FORWARD_DEF(NS_GRAPHICS, class Uniform);
FORWARD_DEF(NS_MEMORY, class MemoryChunk);

NS_ECS
FORWARD_DEF(NS_VIEW, class CameraPerspective);

NS_SYSTEM

class UpdateCameraPerspective
{

public:
	UpdateCameraPerspective(
		std::shared_ptr<memory::MemoryChunk> uniformMemory,
		std::shared_ptr<graphics::MinecraftRenderer>
	);

	void update(
		f32 deltaTime,
		std::shared_ptr<ecs::view::CameraPerspective> view
	);

private:
	std::shared_ptr<graphics::MinecraftRenderer> mpRenderer;
	std::shared_ptr<graphics::Uniform> mpUniform_ChunkViewProjection;
	std::shared_ptr<graphics::Uniform> mpUniform_LocalViewProjection;

};

NS_END
NS_END
