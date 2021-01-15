#pragma once

#include "render/line/LineRenderer.hpp"

NS_GRAPHICS

class SimpleLineRenderer : public graphics::LineRenderer
{

public:
	struct LineSegment
	{
		math::Vector3Padded pos1;
		math::Vector3Padded pos2;
		math::Vector4 color;
	};

	SimpleLineRenderer();

	ui32 addLineSegment(LineSegment const& segment);

protected:

	graphics::AttributeBinding makeVertexBinding(ui8 &slot) const override;
	uSize vertexBufferSize() const override;
	void* vertexBufferData() const override;
	uSize indexBufferSize() const override;
	void* indexBufferData() const override;
	ui32 indexCount() const override;

private:

	struct LineVertex
	{
		math::Vector3Padded position;
		math::Vector4 color;
	};

	std::vector<LineVertex> mVerticies;
	std::vector<ui16> mIndicies;
	ui16 pushVertex(LineVertex vertex);

};

NS_END
