#pragma once

#include "CoreInclude.hpp"

#include "math/Matrix.hpp"
#include "graphics/Buffer.hpp"
#include "world/BlockMetadata.hpp"
#include "world/WorldCoordinate.hpp"
#include "graphics/AttributeBinding.hpp"

FORWARD_DEF(NS_GRAPHICS, class Command)
FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice)
FORWARD_DEF(NS_GRAPHICS, class Memory)

NS_WORLD

class BlockInstanceMap
{
	typedef std::function<void(
		world::Coordinate const&, std::optional<BlockMetadata> const&, std::optional<BlockMetadata> const&
	)> TOnBlockChanged;

public:

	struct RenderData
	{
		math::Vector3Padded posOfChunk; // the coordinate of the chunk this block is in
		math::Matrix4x4 model; // the position of the block in the chunk, as well as any rotation on its local origin
	};

	struct InstanceData
	{
		graphics::Buffer* buffer;
		ui32 offset;
		ui32 count;
	};

	static graphics::AttributeBinding getBinding(ui8& slot);

	BlockInstanceMap();

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device);

	/**
	 * Recreates the instance buffer for the desired size.
	 * Assumes the instance buffer is not currently in use (rendering should be disabled when this function is called).
	 */
	void constructInstanceBuffer(ui8 chunkRenderDistance, ui8 chunkSideLength);

	void invalidate();

	TOnBlockChanged onBlockChangedListener();

	void updateCoordinate(
		world::Coordinate const& global,
		std::optional<BlockMetadata> const& prev,
		std::optional<BlockMetadata> const& next
	);

	void writeInstanceBuffer(graphics::CommandPool* transientPool);

	InstanceData getBlockInstanceData(game::BlockId const &id);

private:
	std::shared_ptr<graphics::Memory> mpInstanceMemory;
	graphics::Buffer mInstanceBuffer;
	ui32 mInstanceCount;

	ui64 getInstanceBufferSize(ui8 chunkRenderDistance, ui8 chunkSideLength) const;

};

NS_END
