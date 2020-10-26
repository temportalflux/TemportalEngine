#pragma once

#include "IRender.hpp"

#include "graphics/AttributeBinding.hpp"
#include "graphics/Buffer.hpp"
#include "world/WorldCoordinate.hpp"
#include "world/BlockMetadata.hpp"

FORWARD_DEF(NS_GRAPHICS, class GameRenderer)
FORWARD_DEF(NS_WORLD, class BlockInstanceMap);
FORWARD_DEF(NS_GAME, class VoxelModelManager);

// System for rendering a set of cubes
class RenderBlocks : public IRender
{

public:
	RenderBlocks(std::weak_ptr<game::VoxelModelManager> registry);

	std::vector<graphics::AttributeBinding> getBindings(ui8 &slot) const;
	// TODO: Should take an array of a custom structure which maps to specific components (transform)
	void init(graphics::GameRenderer *renderer);
	void record(graphics::Command *command) override;

	void invalidate();

	std::function<void(
		world::Coordinate const&, std::optional<BlockMetadata> const&, std::optional<BlockMetadata> const&
	)> onBlockChangedListener();

	void writeInstanceBuffer(graphics::CommandPool* transientPool);

private:
	std::weak_ptr<game::VoxelModelManager> mpBlockRegistry;

	std::shared_ptr<graphics::Memory> mpBufferMemory;

	graphics::Buffer mVertexBuffer;
	
	graphics::Buffer mIndexBuffer;
	ui32 mIndexCount;
	vk::IndexType mIndexBufferUnitType;

	std::shared_ptr<world::BlockInstanceMap> mpBlockRenderInstances;

};
