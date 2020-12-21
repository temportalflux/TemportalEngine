#pragma once

#include "render/IModel.hpp"

#include "graphics/AttributeBinding.hpp"

struct ModelVoxelVertex
{
	math::Vector3Padded position;
	math::Vector2Padded texCoord;
	math::Vector4 flags;
};

class ModelVoxel : public render::IModel<ModelVoxelVertex>
{
public:
	static std::vector<graphics::AttributeBinding> bindings(ui8 &slot);
	static ui32 makeFaceBitMask(ui8 axis, ui8 direction);
	ModelVoxel();

	ModelVoxel& pushRight(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize);
	ModelVoxel& pushLeft(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize);
	ModelVoxel& pushFront(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize);
	ModelVoxel& pushBack(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize);
	ModelVoxel& pushUp(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize);
	ModelVoxel& pushDown(math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize);

private:
	void pushFace(
		math::Vector3 const& uNeg, math::Vector3 const& uPos,
		math::Vector3 const& vNeg, math::Vector3 const& vPo,
		math::Vector3 const& axis,
		math::Vector2 const& texCoordOffset, math::Vector2 const& texCoordSize,
		ui32 const& faceBitMask
	);

};
