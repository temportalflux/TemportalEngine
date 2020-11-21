#include "gui/asset/EditorPipeline.hpp"

#include "asset/AssetManager.hpp"
#include "asset/Shader.hpp"
#include "memory/MemoryChunk.hpp"

#include "property/Property.hpp"

using namespace gui;

std::shared_ptr<AssetEditor> EditorPipeline::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorPipeline>();
}

void EditorPipeline::renderContent()
{
	AssetEditor::renderContent();
	auto asset = this->get<asset::Pipeline>();
	RENDER_ASSET_EDITOR_PROPERTY("Viewport", Viewport);
	RENDER_ASSET_EDITOR_PROPERTY("Scissor", Scissor);
	RENDER_ASSET_EDITOR_PROPERTY("Blend Mode", BlendMode);
	RENDER_ASSET_EDITOR_PROPERTY("Front Face", FrontFace);
	RENDER_ASSET_EDITOR_PROPERTY("Topology", Topology);
	RENDER_ASSET_EDITOR_PROPERTY("Line Width", LineWidth);
	RENDER_ASSET_EDITOR_PROPERTY("Vertex Shader", VertexShader);
	RENDER_ASSET_EDITOR_PROPERTY("Fragment Shader", FragmentShader);
	RENDER_ASSET_EDITOR_PROPERTY("Descriptor Groups", DescriptorGroups);
}
