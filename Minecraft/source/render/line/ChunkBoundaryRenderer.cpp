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
	for (auto const& segment : segments)
	{
		settings.indexCount += this->addLineSegment(segment);
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
