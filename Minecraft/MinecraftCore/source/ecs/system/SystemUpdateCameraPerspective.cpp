#include "ecs/system/SystemUpdateCameraPerspective.hpp"

#include "Engine.hpp"
#include "graphics/Uniform.hpp"
#include "input/Queue.hpp"
#include "logging/Logger.hpp"
#include "math/transform.hpp"
#include "memory/MemoryChunk.hpp"
#include "render/MinecraftRenderer.hpp"

#include "ecs/view/ViewPlayerCamera.hpp"
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

math::Matrix4x4 perspective_RightHand_DepthZeroToOne(
	f32 yFOV, f32 aspectRatio, f32 nearPlane, f32 farPlane
)
{
	/* Based on GLM
	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> perspectiveRH_ZO(T fovy, T aspect, T zNear, T zFar)
	{
		assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		mat<4, 4, T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
		Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
		Result[2][2] = zFar / (zNear - zFar);
		Result[2][3] = - static_cast<T>(1);
		Result[3][2] = -(zFar * zNear) / (zFar - zNear);
		return Result;
	}
	*/

	// A tweet about handedness in different engines: https://twitter.com/FreyaHolmer/status/644881436982575104

	assert(abs(aspectRatio - std::numeric_limits<f32>::epsilon()) > 0.0f);
	f32 const tanHalfFovY = tan(yFOV / 2.0f);
	auto perspective = math::Matrix4x4(0.0f);
	perspective[0][0] = 1.0f / (aspectRatio * tanHalfFovY);
	perspective[1][1] = -1.0f / (tanHalfFovY);
	perspective[2][2] = farPlane / (nearPlane - farPlane);
	perspective[2][3] = -1.0f;
	perspective[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);

	return perspective;
}

UpdateCameraPerspective::UpdateCameraPerspective(
	std::shared_ptr<memory::MemoryChunk> uniformMemory,
	std::shared_ptr<graphics::MinecraftRenderer> renderer
) : System(view::PlayerCamera::TypeId), mpRenderer(renderer), mViewType(EViewType::eFirstPerson)
{
	// TODO: Use dedicated graphics memory
	this->mpUniform_ChunkViewProjection = graphics::Uniform::create<ChunkViewProj>(uniformMemory);
	renderer->addMutableUniform("cameraUniform", this->mpUniform_ChunkViewProjection);
}

void UpdateCameraPerspective::subscribeToQueue()
{
	auto inputQueue = engine::Engine::Get()->getInputQueue();
#define REGISTER_INPUT(EVENT, FUNC_PTR) inputQueue->OnInputEvent.bind(EVENT, this->weak_from_this(), std::bind(FUNC_PTR, this, std::placeholders::_1))
	REGISTER_INPUT(input::EInputType::KEY, &UpdateCameraPerspective::onKeyInput);
#undef REGISTER_INPUT
}

void UpdateCameraPerspective::onKeyInput(input::Event const & evt)
{
	if (evt.inputKey.action == input::EAction::PRESS && evt.inputKey.key == input::EKey::F5)
	{
		this->mViewType = EViewType((ui8(this->mViewType) + 1) % ui8(EViewType::COUNT));
	}
}

void UpdateCameraPerspective::update(f32 deltaTime, std::shared_ptr<ecs::view::View> view)
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

	auto viewMatrix = this->calculateViewMatrix(transform->position(), transform->orientation());
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
}

math::Matrix4x4 UpdateCameraPerspective::calculateViewMatrix(
	world::Coordinate const& position, math::Quaternion orientation
)
{
	OPTICK_EVENT();

	auto cameraViewPos = position.local().toFloat() + position.offset();
	switch (this->mViewType)
	{
	case EViewType::eFirstPerson:
		cameraViewPos += { 0, 1.5f, 0 };
		break;
	case EViewType::eThirdPerson:
		cameraViewPos += orientation.rotate({ 0, 2, 5 });
		break;
	case EViewType::eThirdPersonReverse:
		orientation = math::Quaternion::concat(
			orientation,
			math::Quaternion::FromAxisAngle(math::V3_UP, math::toRadians(180))
		);
		cameraViewPos += orientation.rotate({ 0, 2, 5 });
		break;
	}

	auto fwd = orientation.rotate(math::V3_FORWARD);
	auto up = orientation.rotate(math::V3_UP);
	return math::lookAt(cameraViewPos, cameraViewPos + fwd, up);
}
