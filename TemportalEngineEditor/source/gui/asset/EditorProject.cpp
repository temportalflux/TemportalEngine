#include "gui/asset/EditorProject.hpp"

#include "asset/Project.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"
#include "gui/widget/Optional.hpp"
#include "asset/RenderPassAsset.hpp"

#include <imgui.h>

using namespace gui;

// Macros for the bits each field of the project asset correspond to in the dirty flags
#define Bit_Name 1 << 0
#define Bit_Version 1 << 1
#define Bit_Graphics_GPUPrefs 1 << 2
#define Bit_Graphics_Shader 1 << 3

std::shared_ptr<AssetEditor> EditorProject::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorProject>();
}

void EditorProject::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);

	auto asset = this->get<asset::Project>();

	// General
	{
		this->mFieldName.string(asset->getName());

		this->mSavedVersion = asset->getVersion();
		this->mFieldVersion[0] = this->mSavedVersion.unpacked.major;
		this->mFieldVersion[1] = this->mSavedVersion.unpacked.minor;
		this->mFieldVersion[2] = (ui8)this->mSavedVersion.unpacked.patch;
	}
	// Graphics
	{
		this->mGpuPreference = gui::PropertyGpuPreference(asset->getPhysicalDevicePreference());
	}

	this->mSavedRenderPass = asset->mRenderPass;
	this->mFieldRenderPass.updateAssetList(asset->mRenderPass.getTypeFilter());

}

void EditorProject::renderContent()
{
	AssetEditor::renderContent();

	auto asset = this->get<asset::Project>();
	
	// Name
	if (this->mFieldName.render("Name"))
	{
		this->markAssetDirty(Bit_Name, this->mFieldName.string() != asset->getName());
	}

	if (this->mFieldVersion.render(
		"Version " + this->mSavedVersion.toString() + "###Version",
		0, 0, "%d"
	))
	{
		this->mSavedVersion.unpacked.major = this->mFieldVersion[0];
		this->mSavedVersion.unpacked.minor = this->mFieldVersion[1];
		this->mSavedVersion.unpacked.patch = this->mFieldVersion[2];
		this->markAssetDirty(Bit_Version, this->mSavedVersion.packed != asset->getVersion().packed);
	}

	if (ImGui::TreeNode("Graphics"))
	{
		if (this->mGpuPreference.render("GPU Preferences"))
		{
			this->markAssetDirty(Bit_Graphics_GPUPrefs);
		}
		ImGui::TreePop();
	}

	// TEMPORARY ASSETPATH PROPERTIES
	//auto str = this->mSavedShaderVert.toString();
	//ImGui::InputText("Vertex Shader", str.data(), str.size(), ImGuiInputTextFlags_ReadOnly);
	//ImGui::SameLine();
	if (this->mFieldRenderPass.render("renderPass", "Render Pass", this->mSavedRenderPass.path()))
	{
		this->markAssetDirty(Bit_Graphics_Shader);
	}

}

void EditorProject::saveAsset()
{
	auto asset = this->get<asset::Project>();
	asset->setName(this->mFieldName.string());
	asset->setVersion(this->mSavedVersion);
	asset->setPhysicalDevicePreference(this->mGpuPreference.value());
	asset->mRenderPass = this->mSavedRenderPass;
	AssetEditor::saveAsset();
}
