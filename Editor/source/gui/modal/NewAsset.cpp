#include "gui/modal/NewAsset.hpp"

#include "asset/AssetManager.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>
#include <filesystem>

using namespace gui::modal;

NewAsset::NewAsset() : Modal("New Asset")
{
	this->mInputDirectory.fill('\0');
	this->mInputName.fill('\0');
}

void NewAsset::open()
{
	Modal::open();

	auto assetManager = asset::AssetManager::get();
	this->mAssetTypes = assetManager->getAssetTypes();
	this->mAssetTypeCount = this->mAssetTypes.size();
	this->mAssetTypeDisplayNames.resize(this->mAssetTypeCount);
	std::transform(
		this->mAssetTypes.begin(), this->mAssetTypes.end(), this->mAssetTypeDisplayNames.begin(),
		std::bind(&asset::AssetManager::getAssetTypeDisplayName, assetManager, std::placeholders::_1)
	);
	this->mSelectedAssetType = std::make_pair(*this->mAssetTypes.begin(), 0);
}

void NewAsset::forEachAssetType(std::function<void(std::string type, std::string displayName, uSize idx)> body) const
{
	auto iterAssetType = this->mAssetTypes.begin();
	for (uSize i = 0; i < this->mAssetTypeCount; ++i)
	{
		body(*iterAssetType, this->mAssetTypeDisplayNames[i], i);
		iterAssetType++;
	}
}

void NewAsset::drawContents()
{
	if (ImGui::BeginCombo("Asset Type", this->mAssetTypeDisplayNames[this->mSelectedAssetType.second].c_str()))
	{
		this->forEachAssetType([&](std::string type, std::string displayName, uSize idx)
		{
			bool bIsSelected = (type == this->mSelectedAssetType.first);
			if (ImGui::Selectable(displayName.c_str(), bIsSelected))
			{
				this->mSelectedAssetType = std::make_pair(type, idx);
			}
			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		});
		ImGui::EndCombo();
	}
	
	ImGui::InputText("Directory", this->mInputDirectory.data(), this->mInputDirectory.size());
	ImGui::InputText("Project Name", this->mInputName.data(), this->mInputName.size());
	
	if (ImGui::Button("Create"))
	{
		this->submit();
	}
}

void NewAsset::submit()
{
	auto directory = utility::createStringFromFixedArray(this->mInputDirectory);
	auto name = utility::createStringFromFixedArray(this->mInputName);

	auto assetType = this->mSelectedAssetType.first;
	//LOG.log(logging::ECategory::LOGDEBUG, "Creating %s asset with name %s in %s", type.c_str(), name.c_str(), directory.c_str());

	// Determine the directory path and ensure that it exists
	std::filesystem::path dirPath(directory);
	assert(std::filesystem::exists(dirPath));
	assert(std::filesystem::is_directory(dirPath));

	// Determine the file path and assume it doesn't exist
	// TODO: could prompt to overwrite the file if it does exist
	std::filesystem::path filePath = dirPath / (name + ".te-asset");
	assert(!std::filesystem::exists(filePath));

	auto assetManager = asset::AssetManager::get();
	auto asset = assetManager->createAsset(assetType, filePath);
	asset.reset(); // release the shared_ptr

	this->close();
}

void NewAsset::reset()
{
	Modal::reset();
	this->mInputDirectory.fill('\0');
	this->mInputName.fill('\0');
}
