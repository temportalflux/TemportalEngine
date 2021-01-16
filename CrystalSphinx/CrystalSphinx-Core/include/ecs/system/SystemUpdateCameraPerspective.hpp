#pragma once

#include "ecs/system/System.hpp"

#include "input/Event.hpp"
#include "math/Matrix.hpp"
#include "world/WorldCoordinate.hpp"

FORWARD_DEF(NS_GRAPHICS, class ImmediateRenderSystem);
FORWARD_DEF(NS_GRAPHICS, class Uniform);
FORWARD_DEF(NS_MEMORY, class MemoryChunk);

NS_ECS
FORWARD_DEF(NS_VIEW, class CameraPerspective);

NS_SYSTEM

class UpdateCameraPerspective : public System
{

public:
	UpdateCameraPerspective(
		std::shared_ptr<memory::MemoryChunk> uniformMemory,
		std::shared_ptr<graphics::ImmediateRenderSystem>
	);

	void subscribeToQueue();
	void update(f32 deltaTime, std::shared_ptr<ecs::view::View> view) override;

private:
	std::shared_ptr<graphics::ImmediateRenderSystem> mpRenderer;
	std::shared_ptr<graphics::Uniform> mpUniform_ChunkViewProjection;

	enum class EViewType : ui8
	{
		eFirstPerson = 0,
		eThirdPerson = 1,
		eThirdPersonReverse = 2,
		
		COUNT,
	};
	EViewType mViewType;

	void onKeyInput(input::Event const & evt);
	math::Matrix4x4 calculateViewMatrix(
		world::Coordinate const& position, math::Quaternion orientation
	);

};

NS_END
NS_END
