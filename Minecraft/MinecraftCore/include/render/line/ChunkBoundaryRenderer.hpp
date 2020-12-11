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
	struct LineSegment
	{
		math::Vector3Padded pos1;
		math::Vector3Padded pos2;
		math::Vector4 color;
	};

	ChunkBoundaryRenderer();
	~ChunkBoundaryRenderer();

	void setBoundarySegments(ChunkBoundaryType boundaryType, std::vector<LineSegment> const& segments, bool bEnabledByDefault);
	bool isBoundaryEnabled(ChunkBoundaryType boundaryType) const;
	void setIsBoundaryEnabled(ChunkBoundaryType boundaryType, bool bRender);

	void draw(graphics::Command *command) override;

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
		// If a given dimension is 0, the vertex is rendered in world space.
		// If it is 1, the vertex is rendered in chunk space.
		math::Vector3Padded chunkSpaceMask;
	};

	std::vector<LineVertex> mVerticies;
	std::vector<ui16> mIndicies;

	ui32 addLineSegment(LineSegment const& segmentl, math::Vector3 const& chunkSpaceMask);
	ui16 pushVertex(LineVertex vertex);
	
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
