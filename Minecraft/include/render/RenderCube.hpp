#pragma once

#include "IRender.hpp"

#include "graphics/AttributeBinding.hpp"
#include "graphics/Buffer.hpp"
#include "world/WorldCoordinate.hpp"

FORWARD_DEF(NS_GRAPHICS, class GameRenderer)

// System for rendering a set of cubes
class RenderCube : public IRender
{

public:
	RenderCube();

	std::vector<graphics::AttributeBinding> getBindings(ui8 &slot) const;
	// TODO: Should take an array of a custom structure which maps to specific components (transform)
	void init(graphics::GameRenderer *renderer, std::vector<world::Coordinate> const &instances);
	void draw(graphics::Command *command) override;

	void invalidate();

private:
	std::shared_ptr<graphics::Memory> mpBufferMemory;

	graphics::Buffer mVertexBuffer;

	graphics::Buffer mInstanceBuffer;
	ui32 mInstanceCount;
	
	graphics::Buffer mIndexBuffer;
	ui32 mIndexCount;
	vk::IndexType mIndexBufferUnitType;

};
