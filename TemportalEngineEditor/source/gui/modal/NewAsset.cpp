#include "gui/modal/NewAsset.hpp"

#include "Editor.hpp"
#include "asset/AssetManager.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>
#include <filesystem>

using namespace gui::modal;

NewAsset::NewAsset(const char* title) : Modal(title)
{
	this->mForcedAssetType = std::nullopt;
	this->mInputDirectory.fill('\0');
	this->mInputName.fill('\0');
}

void NewAsset::setAssetType(asset::AssetType type)
{
	this->mForcedAssetType = type;
}

void NewAsset::setCallback(AssetCreatedCallback callback)
{
	this->mOnAssetCreated = callback;
}

NewAsset& NewAsset::setDirectory(std::filesystem::path const &path)
{
	this->mInputDirectory.fill('\0');
	std::copy_n(path.string().begin(), path.string().length(), this->mInputDirectory.begin());
	return *this;
}

void NewAsset::open()
{
	Modal::open();

	if (!this->mForcedAssetType.has_value())
	{
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
	if (!this->mForcedAssetType.has_value() && ImGui::BeginCombo("Asset Type", this->mAssetTypeDisplayNames[this->mSelectedAssetType.second].c_str()))
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
	ImGui::InputText("Name", this->mInputName.data(), this->mInputName.size());
	
	if (ImGui::Button("Create"))
	{
		this->submit();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		this->close();
	}
}

void NewAsset::submit()
{
	auto directory = utility::createStringFromFixedArray(this->mInputDirectory);
	auto name = utility::createStringFromFixedArray(this->mInputName);

	auto assetManager = asset::AssetManager::get();
	auto assetType = this->mForcedAssetType.has_value() ? this->mForcedAssetType.value() : this->mSelectedAssetType.first;
	//LOG.log(logging::ECategory::LOGDEBUG, "Creating %s asset with name %s in %s", type.c_str(), name.c_str(), directory.c_str());

	// Determine the directory path and ensure that it exists
	std::filesystem::path dirPath(directory);
	if (dirPath.is_relative()) dirPath = Editor::EDITOR->getProject()->getAbsoluteDirectoryPath() / dirPath;
	assert(std::filesystem::exists(dirPath));
	assert(std::filesystem::is_directory(dirPath));

	// Determine the file path and assume it doesn't exist
	// TODO: could prompt to overwrite the file if it does exist
	std::filesystem::path filePath = dirPath / (name + assetManager->getAssetTypeMetadata(assetType).fileExtension);
	assert(!std::filesystem::exists(filePath));

	auto asset = assetManager->createAsset(assetType, filePath);
	this->close();
	this->mOnAssetCreated(asset);
}

void NewAsset::reset()
{
	Modal::reset();
	this->mInputDirectory.fill('\0');
	this->mInputName.fill('\0');
}
