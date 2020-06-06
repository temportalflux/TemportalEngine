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

	this->mFieldName.string(asset->getName());

	this->mSavedVersion = asset->getVersion();
	this->mFieldVersion[0] = this->mSavedVersion.unpacked.major;
	this->mFieldVersion[1] = this->mSavedVersion.unpacked.minor;
	this->mFieldVersion[2] = this->mSavedVersion.unpacked.patch;
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
		this->mSavedVersion.unpacked.major = (ui8)this->mFieldVersion[0];
		this->mSavedVersion.unpacked.minor = (ui8)this->mFieldVersion[1];
		this->mSavedVersion.unpacked.patch = (ui8)this->mFieldVersion[2];
		this->markAssetDirty(Bit_Version, this->mSavedVersion.packed != asset->getVersion().packed);
	}
}

void EditorProject::saveAsset()
{
	auto asset = this->get<asset::Project>();
	asset->setName(this->mFieldName.string());
	asset->setVersion(this->mSavedVersion);
	AssetEditor::saveAsset();
}
