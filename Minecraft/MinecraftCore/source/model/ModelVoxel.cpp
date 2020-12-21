#include "model/ModelVoxel.hpp"

#include <vulkan/vulkan.hpp>

std::vector<graphics::AttributeBinding> ModelVoxel::bindings(ui8 &slot)
{
	return {
		// Data per vertex of object instance
		graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
		.setStructType<ModelVoxelVertex>()
		.addAttribute({ /*slot*/ slot++, /*vec3*/ (ui32)vk::Format::eR32G32B32Sfloat, offsetof(ModelVoxelVertex, position) })
		.addAttribute({ /*slot*/ slot++, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(ModelVoxelVertex, texCoord) })
		.addAttribute({ /*slot*/ slot++, /*vec4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(ModelVoxelVertex, flags) })
	};
}

ui32 ModelVoxel::makeFaceBitMask(ui8 axis, ui8 direction)
{
	return 1u << ((axis * 2) + direction);
}

ModelVoxel::ModelVoxel()
{
}

ModelVoxel& ModelVoxel::pushRight(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize)
{
	this->pushFace(
		/*U = Front -> Back*/ math::Vector3::ZERO, -math::V3_FORWARD,
		/*V = Top -> Bottom*/ math::V3_UP, math::Vector3::ZERO,
		math::V3_RIGHT,
		texCoordOffset, texCoordSize,
		makeFaceBitMask(/*x*/ 0, /*pos*/ 1)
	);
	return *this;
}

ModelVoxel& ModelVoxel::pushLeft(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize)
{
	this->pushFace(
		/*U = Back -> Front*/ -math::V3_FORWARD, math::Vector3::ZERO,
		/*V = Top -> Bottom*/ math::V3_UP, math::Vector3::ZERO,
		math::Vector3::ZERO,
		texCoordOffset, texCoordSize,
		makeFaceBitMask(/*x*/ 0, /*neg*/ 0)
	);
	return *this;
}

ModelVoxel& ModelVoxel::pushFront(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize)
{
	this->pushFace(
		/*U = Left -> Right*/ math::Vector3::ZERO, math::V3_RIGHT,
		/*V = Top -> Bottom*/ math::V3_UP, math::Vector3::ZERO,
		math::Vector3::ZERO,
		texCoordOffset, texCoordSize,
		makeFaceBitMask(/*z*/ 2, /*neg*/ 0)
	);
	return *this;
}

ModelVoxel& ModelVoxel::pushBack(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize)
{
	this->pushFace(
		/*U = Right -> Left*/ math::V3_RIGHT, math::Vector3::ZERO,
		/*V = Top -> Bottom*/ math::V3_UP, math::Vector3::ZERO,
		-math::V3_FORWARD,
		texCoordOffset, texCoordSize,
		makeFaceBitMask(/*z*/ 2, /*pos*/ 1)
	);
	return *this;
}

ModelVoxel& ModelVoxel::pushUp(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize)
{
	this->pushFace(
		/*U = Left -> Right*/ math::Vector3::ZERO, math::V3_RIGHT,
		/*V = Front -> Back*/ -math::V3_FORWARD, math::Vector3::ZERO,
		math::V3_UP,
		texCoordOffset, texCoordSize,
		makeFaceBitMask(/*y*/ 1, /*pos*/ 1)
	);
	return *this;
}

ModelVoxel& ModelVoxel::pushDown(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize)
{
	this->pushFace(
		/*U = Left -> Right*/ math::Vector3::ZERO, math::V3_RIGHT,
		/*V = Back -> Front*/ math::Vector3::ZERO, -math::V3_FORWARD,
		math::Vector3::ZERO,
		texCoordOffset, texCoordSize,
		makeFaceBitMask(/*y*/ 1, /*neg*/ 0)
	);
	return *this;
}

void ModelVoxel::pushFace(
	math::Vector3 const& uNeg, math::Vector3 const& uPos,
	math::Vector3 const& vNeg, math::Vector3 const& vPos,
	math::Vector3 const& axis,
	math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize,
	ui32 const& faceBitMask
)
{
	math::Vector4 flags = { *reinterpret_cast<f32 const*>(&faceBitMask) };
	auto idxTL = this->pushVertex({
		uNeg + vNeg + axis,
		texCoordOffset + texCoordSize * math::Vector2({ 0.0f, 0.0f }),
		flags
	});
	auto idxTR = this->pushVertex({
		uPos + vNeg + axis,
		texCoordOffset + texCoordSize * math::Vector2({ 1.0f, 0.0f }),
		flags
	});
	auto idxBL = this->pushVertex({
		uNeg + vPos + axis,
		texCoordOffset + texCoordSize * math::Vector2({ 0.0f, 1.0f }),
		flags
	});
	auto idxBR = this->pushVertex({
		uPos + vPos + axis,
		texCoordOffset + texCoordSize * math::Vector2({ 1.0f, 1.0f }),
		flags
	});
	this->pushTri({ idxTL, idxTR, idxBR });
	this->pushTri({ idxBR, idxBL, idxTL });
}
