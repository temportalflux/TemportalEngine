#pragma once

#include "render/line/LineRenderer.hpp"
#include "thread/MutexLock.hpp"

NS_GRAPHICS

enum class ChunkBoundaryType : ui32
{
	eColumn = 0,
	eCube = 1,
	eSideGrid = 2,

	Count,
};

class ChunkBoundaryRenderer : public LineRenderer
{

public:
	ChunkBoundaryRenderer(std::weak_ptr<graphics::DescriptorPool> pDescriptorPool);
	~ChunkBoundaryRenderer();

	void setBoundarySegments(ChunkBoundaryType boundaryType, std::vector<LineSegment> const& segments, bool bEnabledByDefault);
	void toggleBoundaryRender(ChunkBoundaryType boundaryType);

	void draw(graphics::Command *command) override;

private:
	struct BoundarySettings
	{
		ui32 firstIndex;
		ui32 indexCount;
		bool bShouldRender;
		thread::MutexLock mutex;
	};
	std::unordered_map<ChunkBoundaryType, BoundarySettings> mBoundarySettings;

};

NS_END
