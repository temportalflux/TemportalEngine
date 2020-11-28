#include "ecs/system/SystemUpdateCameraPerspective.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"
#include "graphics/Uniform.hpp"
#include "memory/MemoryChunk.hpp"
#include "render/MinecraftRenderer.hpp"

#include "ecs/view/ViewCameraPerspective.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentCameraPOV.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // perspective needs to use [0,1] range for Vulkan
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace ecs;
using namespace ecs::system;

// UniformBufferObject (UBO) for turning world coordinates to clip space when rendering
struct ChunkViewProj
{
	math::Matrix4x4 view;
	glm::mat4 proj;
	math::Vector3Padded posOfCurrentChunk;
	math::Vector3Padded sizeOfChunkInBlocks;

	ChunkViewProj()
	{
		view = math::Matrix4x4(1);
		proj = glm::mat4(1);
		posOfCurrentChunk = math::Vector3Padded({ 0, 0, 0 });
		sizeOfChunkInBlocks = math::Vector3Padded({ CHUNK_SIDE_LENGTH, CHUNK_SIDE_LENGTH, CHUNK_SIDE_LENGTH });
	}
};

struct LocalCamera
{
	math::Matrix4x4 view;
	glm::mat4 proj;

	LocalCamera()
	{
		view = math::Matrix4x4(1);
		proj = glm::mat4(1);
	}
};

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
	static logging::Logger ControllerLog = DeclareLog("Controller");

	auto transform = view->get<component::CoordinateTransform>();
	auto cameraPOV = view->get<component::CameraPOV>();
	assert(transform && cameraPOV);

	OPTICK_EVENT();

	// TODO: Eliminate glm
	auto perspective = glm::perspective(glm::radians(cameraPOV->fov()), this->mpRenderer->getAspectRatio(), /*near plane*/ 0.01f, /*far plane*/ 100.0f);
	// GLM by default is not Y-up Right-Handed, so we have to flip the x and y coord bits
	// that said, this tweet says differently... https://twitter.com/FreyaHolmer/status/644881436982575104
	perspective[1][1] *= -1;
	perspective[0][0] *= -1;

	auto viewMatrix = transform->calculateView();

	// Chunk View Projection
	{
		auto uniData = this->mpUniform_ChunkViewProjection->read<ChunkViewProj>();
		uniData.view = viewMatrix;
		uniData.proj = perspective;
		uniData.posOfCurrentChunk = transform->position().chunk().toFloat();
		this->mpUniform_ChunkViewProjection->write(&uniData);
	}

	// Local View Projection
	{
		auto localCamera = this->mpUniform_LocalViewProjection->read<LocalCamera>();
		localCamera.view = viewMatrix;
		localCamera.proj = perspective;
		this->mpUniform_LocalViewProjection->write(&localCamera);
	}
}
