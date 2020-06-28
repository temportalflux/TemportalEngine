#pragma once

#include "build/asset/BuildAsset.hpp"

#include "graphics/FontGlyph.hpp"

NS_BUILD

class BuildFont : public BuildAsset
{

public:
	static std::shared_ptr<BuildAsset> create(std::shared_ptr<asset::Asset> asset);

	BuildFont() = default;
	BuildFont(std::shared_ptr<asset::Asset> asset);

	std::vector<std::string> compile() override;
	void save() override;

private:
	std::vector<graphics::FontGlyphSet> mGlyphSets;

};

NS_END
