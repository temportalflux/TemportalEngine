#include "render/line/SimpleLineRenderer.hpp"

using namespace graphics;

SimpleLineRenderer::SimpleLineRenderer(std::weak_ptr<graphics::DescriptorPool> pDescriptorPool) : LineRenderer(pDescriptorPool)
{
}

graphics::AttributeBinding SimpleLineRenderer::makeVertexBinding(ui8 &slot) const
{
	return graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
		.setStructType<LineVertex>()
		.addAttribute({ slot++, /*vec3*/(ui32)vk::Format::eR32G32B32Sfloat, offsetof(LineVertex, position) })
		.addAttribute({ slot++, /*vec4*/(ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(LineVertex, color) });
}

ui32 SimpleLineRenderer::addLineSegment(LineSegment const& segment)
{
	this->mIndicies.push_back(this->pushVertex({ segment.pos1, segment.color }));
	this->mIndicies.push_back(this->pushVertex({ segment.pos2, segment.color }));
	return 2;
}

ui16 SimpleLineRenderer::pushVertex(LineVertex vertex)
{
	auto i = (ui16)this->mVerticies.size();
	this->mVerticies.push_back(vertex);
	return i;
}

uSize SimpleLineRenderer::vertexBufferSize() const
{
	return this->mVerticies.size() * sizeof(LineVertex);
}

void* SimpleLineRenderer::vertexBufferData() const
{
	return (void*)this->mVerticies.data();
}

uSize SimpleLineRenderer::indexBufferSize() const
{
	return this->mIndicies.size() * sizeof(ui16);
}

void* SimpleLineRenderer::indexBufferData() const
{
	return (void*)this->mIndicies.data();
}

ui32 SimpleLineRenderer::indexCount() const
{
	return (ui32)this->mIndicies.size();
}
