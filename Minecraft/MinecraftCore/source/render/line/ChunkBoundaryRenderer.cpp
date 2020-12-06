#include "render/line/ChunkBoundaryRenderer.hpp"

#include "graphics/Command.hpp"

using namespace graphics;

ChunkBoundaryRenderer::ChunkBoundaryRenderer(std::weak_ptr<graphics::DescriptorPool> pDescriptorPool)
	: LineRenderer(pDescriptorPool)
{
}

ChunkBoundaryRenderer::~ChunkBoundaryRenderer()
{

}

void ChunkBoundaryRenderer::setBoundarySegments(ChunkBoundaryType boundaryType, std::vector<LineSegment> const& segments, bool bEnabledByDefault)
{
	BoundarySettings settings = { this->indexCount(), 0, bEnabledByDefault, thread::MutexLock() };
	
	math::Vector3 chunkSpaceMask = math::Vector3(1);
	if (boundaryType == ChunkBoundaryType::eColumn)
	{
		chunkSpaceMask.y() = 0;
	}
	
	for (auto const& segment : segments)
	{
		settings.indexCount += this->addLineSegment(segment, chunkSpaceMask);
	}
	this->mBoundarySettings.insert(std::make_pair(boundaryType, settings));
}

bool ChunkBoundaryRenderer::isBoundaryEnabled(ChunkBoundaryType boundaryType) const
{
	auto iter = this->mBoundarySettings.find(boundaryType);
	assert(iter != this->mBoundarySettings.end());
	auto& boundary = iter->second;
	return boundary.bShouldRender;
}

// This will execute on the main/input thread but affects the render thread
// Assumes that the command buffer will be re-recorded to (because blocks need to anyway) each frame
void ChunkBoundaryRenderer::setIsBoundaryEnabled(ChunkBoundaryType boundaryType, bool bRender)
{
	auto iter = this->mBoundarySettings.find(boundaryType);
	assert(iter != this->mBoundarySettings.end());
	auto& boundary = iter->second;
	boundary.mutex.lock();
	boundary.bShouldRender = bRender;
	boundary.mutex.unlock();
}

graphics::AttributeBinding ChunkBoundaryRenderer::makeVertexBinding(ui8 &slot) const
{
	return graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
		.setStructType<LineVertex>()
		.addAttribute({ slot++, /*vec3*/(ui32)vk::Format::eR32G32B32Sfloat, offsetof(LineVertex, position) })
		.addAttribute({ slot++, /*vec4*/(ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(LineVertex, color) })
		.addAttribute({ slot++, /*vec3*/(ui32)vk::Format::eR32G32B32Sfloat, offsetof(LineVertex, chunkSpaceMask) });
}

ui32 ChunkBoundaryRenderer::addLineSegment(LineSegment const& segment, math::Vector3 const& chunkSpaceMask)
{
	this->mIndicies.push_back(this->pushVertex({ segment.pos1, segment.color, chunkSpaceMask }));
	this->mIndicies.push_back(this->pushVertex({ segment.pos2, segment.color, chunkSpaceMask }));
	return 2;
}

ui16 ChunkBoundaryRenderer::pushVertex(LineVertex vertex)
{
	auto i = (ui16)this->mVerticies.size();
	this->mVerticies.push_back(vertex);
	return i;
}

uSize ChunkBoundaryRenderer::vertexBufferSize() const
{
	return this->mVerticies.size() * sizeof(LineVertex);
}

void* ChunkBoundaryRenderer::vertexBufferData() const
{
	return (void*)this->mVerticies.data();
}

uSize ChunkBoundaryRenderer::indexBufferSize() const
{
	return this->mIndicies.size() * sizeof(ui16);
}

void* ChunkBoundaryRenderer::indexBufferData() const
{
	return (void*)this->mIndicies.data();
}

ui32 ChunkBoundaryRenderer::indexCount() const
{
	return (ui32)this->mIndicies.size();
}

void ChunkBoundaryRenderer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
	for (uIndex idxSet = 0; idxSet < this->mDescriptorSets.size(); ++idxSet)
	{
		this->mDescriptorSets[idxSet].attach("mvpCamera", mutableUniforms["mvpUniform"][idxSet]);
	}
}

void ChunkBoundaryRenderer::draw(graphics::Command *command)
{
	OPTICK_EVENT();
	OPTICK_GPU_EVENT("DrawChunkBoundaries");
	for (auto& entry : this->mBoundarySettings)
	{
		auto& boundary = entry.second;
		boundary.mutex.lock();
		if (!boundary.bShouldRender)
		{
			boundary.mutex.unlock();
			continue;
		}
		command->draw(
			boundary.firstIndex, boundary.indexCount,
			0, // index shift
			0, 1 // only a single instance, no instance buffer
		);
		boundary.mutex.unlock();
	}

}
