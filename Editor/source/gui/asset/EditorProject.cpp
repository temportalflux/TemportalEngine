#include "gui/asset/EditorProject.hpp"

#include "asset/Project.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"
#include "gui/widget/Optional.hpp"

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
	this->mFieldVersion[2] = (ui8)this->mSavedVersion.unpacked.patch;

	{
		this->mListIntegerTest.setValueRenderer(std::bind(&EditorProject::renderIntegerTestItem, this, std::placeholders::_1));
		auto items = std::vector<ui32>({
			4, 2, 5
		});
		this->mListIntegerTest.value(items);
	}
	{
		this->mListMapTest.setKeyRenderer(std::bind(&EditorProject::renderMapTestKey, this, std::placeholders::_1));
		this->mListMapTest.setValueRenderer(std::bind(&EditorProject::renderMapTestValue, this, std::placeholders::_1));
		auto items = std::map<std::string, std::optional<ui32>>({
			{ "t1", std::nullopt },
			{ "t2", 42 }
		});
		this->mListMapTest.value(items);
	}

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

	if (this->mListIntegerTest.render("ITL", "Integer Test List", /*bItemsCollapse*/ false))
	{

	}

	if (this->mListMapTest.render("MTL", "Map Test List", /*bItemsCollapse*/ true))
	{

	}
}

bool EditorProject::renderIntegerTestItem(ui32 &item)
{
	return gui::FieldNumber<ui32, 1>::InlineSingle("##value", item);
}

bool EditorProject::renderMapTestKey(std::pair<std::string, std::optional<ui32>> &item)
{
	return gui::FieldText<32>::Inline("##value", item.first);
}

bool EditorProject::renderMapTestValue(std::pair<std::string, std::optional<ui32>> &item)
{
	return gui::Optional<ui32>::Inline("##value", item.second, "Required", false, [](ui32 &value) {
		ImGui::SameLine();
		return gui::FieldNumber<ui32, 1>::InlineSingle("##value", value);
	});
}

void EditorProject::saveAsset()
{
	auto asset = this->get<asset::Project>();
	asset->setName(this->mFieldName.string());
	asset->setVersion(this->mSavedVersion);
	AssetEditor::saveAsset();
}
