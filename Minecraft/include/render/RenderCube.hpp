#pragma once

#include "IRender.hpp"

#include "graphics/AttributeBinding.hpp"
#include "graphics/Buffer.hpp"

FORWARD_DEF(NS_GRAPHICS, class GameRenderer)

// System for rendering a set of cubes
class RenderCube : public IRender
{

public:
	RenderCube();

	void appendBindings(std::vector<graphics::AttributeBinding> &bindings, ui8 &slot) const;
	// TODO: Should take an array of a custom structure which maps to specific components (transform)
	void init(graphics::GameRenderer *renderer, std::vector<WorldObject> const &instances);
	void draw(graphics::Command *command) override;

	void invalidate();

private:
	// TODO: system should not hold references to objects
	std::vector<WorldObject> instances;

	std::shared_ptr<graphics::Memory> mpBufferMemory;

	graphics::Buffer mVertexBuffer;

	graphics::Buffer mInstanceBuffer;
	ui32 mInstanceCount;
	
	graphics::Buffer mIndexBuffer;
	ui32 mIndexCount;
	vk::IndexType mIndexBufferUnitType;

};
