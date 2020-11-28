#include "ecs/system/SystemUpdateCameraPerspective.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"
#include "graphics/Uniform.hpp"
#include "memory/MemoryChunk.hpp"
#include "render/MinecraftRenderer.hpp"

#include "ecs/view/ViewCameraPerspective.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentCameraPOV.hpp"

using namespace ecs;
using namespace ecs::system;

// UniformBufferObject (UBO) for turning world coordinates to clip space when rendering
struct ChunkViewProj
{
	math::Matrix4x4 view;
	math::Matrix4x4 proj;
	math::Vector3Padded posOfCurrentChunk;
	math::Vector3Padded sizeOfChunkInBlocks;

	ChunkViewProj()
	{
		view = math::Matrix4x4(1);
		proj = math::Matrix4x4(1);
		posOfCurrentChunk = math::Vector3Padded({ 0, 0, 0 });
		sizeOfChunkInBlocks = math::Vector3Padded({ CHUNK_SIDE_LENGTH, CHUNK_SIDE_LENGTH, CHUNK_SIDE_LENGTH });
	}
};

struct LocalCamera
{
	math::Matrix4x4 view;
	math::Matrix4x4 proj;

	LocalCamera()
	{
		view = math::Matrix4x4(1);
		proj = math::Matrix4x4(1);
	}
};

math::Matrix4x4 perspective_RightHand_DepthZeroToOne(
	f32 yFOV, f32 aspectRatio, f32 nearPlane, f32 farPlane
)
{
	/* Based on GLM
		template<typename T>
		GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> perspectiveRH_NO(T fovy, T aspect, T zNear, T zFar)
		{
			assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

			T const tanHalfFovy = tan(fovy / static_cast<T>(2));

			mat<4, 4, T, defaultp> Result(static_cast<T>(0));
			Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
			Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
			Result[2][2] = - (zFar + zNear) / (zFar - zNear);
			Result[2][3] = - static_cast<T>(1);
			Result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
			return Result;
		}
	*/

	// A tweet about handedness in different engines: https://twitter.com/FreyaHolmer/status/644881436982575104

	assert(abs(aspectRatio - std::numeric_limits<f32>::epsilon()) > 0.0f);
	f32 const tanHalfFovY = tan(yFOV / 2.0f);
	auto perspective = math::Matrix4x4(0.0f);
	perspective[0][0] = -1.0f / (aspectRatio * tanHalfFovY);
	perspective[1][1] = -1.0f / (tanHalfFovY);
	perspective[2][2] = farPlane / (nearPlane - farPlane);
	perspective[2][3] = -1.0f;
	perspective[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);

	return perspective;
}

UpdateCameraPerspective::UpdateCameraPerspective(
	std::shared_ptr<memory::MemoryChunk> uniformMemory,
	std::shared_ptr<graphics::MinecraftRenderer> renderer
) : mpRenderer(renderer)
{
	// TODO: Use dedicated graphics memory
	this->mpUniform_ChunkViewProjection = graphics::Uniform::create<ChunkViewProj>(uniformMemory);
	renderer->addMutableUniform("mvpUniform", this->mpUniform_ChunkViewProjection);

	this->mpUniform_LocalViewProjection = graphics::Uniform::create<LocalCamera>(uniformMemory);
	renderer->addMutableUniform("localCamera", this->mpUniform_LocalViewProjection);
}

void UpdateCameraPerspective::update(
	f32 deltaTime,
	std::shared_ptr<ecs::view::CameraPerspective> view
)
{
	OPTICK_EVENT();

	auto transform = view->get<component::CoordinateTransform>();
	auto cameraPOV = view->get<component::CameraPOV>();
	assert(transform && cameraPOV);

	auto xyAspectRatio = this->mpRenderer->getAspectRatio(); // x/y
	auto verticalFOV = math::toRadians(cameraPOV->fov());
	// According to this calculator http://themetalmuncher.github.io/fov-calc/
	// whose source code is https://github.com/themetalmuncher/fov-calc/blob/gh-pages/index.html#L24
	// the equation to get verticalFOV from horizontalFOV is: verticalFOV = 2 * atan(tan(horizontalFOV / 2) * height / width)
	// And by shifting the math to get horizontal from vertical, the equation is actually the same except the aspectRatio is flipped.
	auto horizontalFOV = 2.0f * atan(tan(verticalFOV / 2.0f) * xyAspectRatio);

	auto viewMatrix = transform->calculateView();
	auto perspectiveMatrix = perspective_RightHand_DepthZeroToOne(
		horizontalFOV, xyAspectRatio,
		cameraPOV->nearPlane(), cameraPOV->farPlane()
	);

	// Chunk View Projection
	{
		auto uniData = this->mpUniform_ChunkViewProjection->read<ChunkViewProj>();
		uniData.view = viewMatrix;
		uniData.proj = perspectiveMatrix;
		uniData.posOfCurrentChunk = transform->position().chunk().toFloat();
		this->mpUniform_ChunkViewProjection->write(&uniData);
	}

	// Local View Projection
	{
		auto localCamera = this->mpUniform_LocalViewProjection->read<LocalCamera>();
		localCamera.view = viewMatrix;
		localCamera.proj = perspectiveMatrix;
		this->mpUniform_LocalViewProjection->write(&localCamera);
	}
}
