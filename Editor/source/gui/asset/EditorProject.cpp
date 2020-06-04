#include "gui/asset/EditorProject.hpp"

#include "asset/Project.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui;

// Macros for the bits each field of the project asset correspond to in the dirty flags
#define Bit_Name 1 << 0
#define Bit_Version 1 << 1

std::shared_ptr<AssetEditor> EditorProject::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorProject>();
}

void EditorProject::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);

	auto asset = this->get<asset::Project>();

	// copy to name
	memcpy(this->mInputName.data(), asset->getName().data(), asset->getName().length());

	std::string versionItem;
	Version version = asset->getVersion();
	// copy version major
	versionItem = std::to_string(version.unpacked.major);
	memcpy(this->mInputVersionMajor.data(), versionItem.data(), versionItem.length());
	// copy version minor
	versionItem = std::to_string(version.unpacked.minor);
	memcpy(this->mInputVersionMinor.data(), versionItem.data(), versionItem.length());
	// copy version patch
	versionItem = std::to_string(version.unpacked.patch);
	memcpy(this->mInputVersionPatch.data(), versionItem.data(), versionItem.length());

}

std::string EditorProject::getInputName() const
{
	return utility::createStringFromFixedArray(this->mInputName);
}

Version EditorProject::getVersion() const
{
	Version v;
	std::string vStr;
	vStr = utility::createStringFromFixedArray(this->mInputVersionMajor);
	v.unpacked.major = vStr.length() > 0 ? std::stoi(vStr) : 0;
	vStr = utility::createStringFromFixedArray(this->mInputVersionMinor);
	v.unpacked.minor = vStr.length() > 0 ? std::stoi(vStr) : 0;
	vStr = utility::createStringFromFixedArray(this->mInputVersionPatch);
	v.unpacked.patch = vStr.length() > 0 ? std::stoi(vStr) : 0;
	return v;
}

void EditorProject::renderView()
{
	AssetEditor::renderView();

	auto asset = this->get<asset::Project>();
	
	// Name
	if (ImGui::InputText("Name", this->mInputName.data(), this->mInputName.size()))
	{
		this->markAssetDirty(Bit_Name, this->getInputName() != asset->getName());
	}
	
	// Version
	bool bAnyVersionChanged = false;
	ImGui::Text("Version (%s.%s.%s)", this->mInputVersionMajor.data(), this->mInputVersionMinor.data(), this->mInputVersionPatch.data());
	//ImGui::SameLine();
	if (ImGui::InputText("##Major", this->mInputVersionMajor.data(), this->mInputVersionMajor.size())) bAnyVersionChanged = true;
	//ImGui::SameLine();
	if (ImGui::InputText("##Minor", this->mInputVersionMinor.data(), this->mInputVersionMinor.size())) bAnyVersionChanged = true;
	//ImGui::SameLine();
	if (ImGui::InputText("##Patch", this->mInputVersionPatch.data(), this->mInputVersionPatch.size())) bAnyVersionChanged = true;
	if (bAnyVersionChanged) this->markAssetDirty(Bit_Version, this->getVersion().packed != asset->getVersion().packed);

}

void EditorProject::saveAsset()
{
	auto asset = this->get<asset::Project>();
	asset->setName(this->getInputName());
	asset->setVersion(this->getVersion());
	AssetEditor::saveAsset();
}
