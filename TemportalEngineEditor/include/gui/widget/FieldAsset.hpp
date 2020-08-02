#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/AssetPath.hpp"

NS_GUI

// TODO: This may need a search functionality in the future
class FieldAsset
{
public:
	FieldAsset() = default;

	void updateAssetList(std::optional<asset::AssetType> filter = std::nullopt);
	bool render(char const* id, std::string title, asset::AssetPath &selected);

	static bool Inline(std::string titleId, asset::AssetPath &selected, std::vector<asset::AssetPath> const& options);

private:
	std::optional<asset::AssetType> mTypeFilter;
	std::vector<asset::AssetPath> mAssetPaths;

};

NS_END
