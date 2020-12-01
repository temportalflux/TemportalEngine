#include "gui/asset/EditorProject.hpp"

#include "asset/Project.hpp"
#include "asset/RenderPassAsset.hpp"
#include "memory/MemoryChunk.hpp"
#include "property/Property.hpp"

using namespace gui;

std::shared_ptr<AssetEditor> EditorProject::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorProject>();
}

void EditorProject::renderContent()
{
	AssetEditor::renderContent();
	auto asset = this->get<asset::Project>();
	RENDER_ASSET_EDITOR_PROPERTY("Name", Name);
	RENDER_ASSET_EDITOR_PROPERTY("Version", Version);
	RENDER_ASSET_EDITOR_PROPERTY("GPU Preferences", PhysicalDevicePreference);
	RENDER_ASSET_EDITOR_PROPERTY("Render Pass", RenderPass);
}
