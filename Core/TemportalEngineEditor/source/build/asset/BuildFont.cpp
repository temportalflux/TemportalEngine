#include "build/asset/BuildFont.hpp"

#include "asset/Font.hpp"
#include "math/Vector.hpp"
#include "build/asset/BuildTexture.hpp"

#include <regex>

using namespace build;

std::shared_ptr<BuildAsset> BuildFont::create(std::shared_ptr<asset::Asset> asset)
{
	return std::make_shared<BuildFont>(asset);
}

BuildFont::BuildFont(std::shared_ptr<asset::Asset> asset) : BuildAsset(asset)
{
}

std::vector<std::string> BuildFont::compile(logging::Logger &logger)
{
	static auto PATTERN_INFO = std::regex("info.*");
	static auto PATTERN_COMMON = std::regex("common[\\s]+lineHeight=([^\\s]+)[\\s]+base=([^\\s]+)[\\s]+.*");
	static auto PATTERN_PAGE = std::regex("page id=[0-9]+ file=\"(.*)\"");
	static auto PATTERN_CHARCOUNT = std::regex("chars count=([0-9]+)");
	static auto PATTERN_GLYPH = std::regex("char[\\s]+id=([^\\s]+)[\\s]+x=([^\\s]+)[\\s]+y=([^\\s]+)[\\s]+width=([^\\s]+)[\\s]+height=([^\\s]+)[\\s]+xoffset=([^\\s]+)[\\s]+yoffset=([^\\s]+)[\\s]+xadvance=([^\\s]+)[\\s]+page=([^\\s]+)[\\s]+chnl=([^\\s]+).*");

	auto errors = std::vector<std::string>();
	auto asset = this->get<asset::Font>();
	auto fntPath = asset->getFontPath();

	auto atlasSize = math::Vector2UInt();
	auto atlasPixels = std::vector<ui8>();
	auto glyphs = std::vector<asset::Font::Glyph>();

	{
		auto stream = std::ifstream(fntPath.string().c_str(), std::ios::binary);
		std::string line;
		std::cmatch match;

		getline(stream, line);
		assert(std::regex_search(line.c_str(), match, PATTERN_INFO));
		getline(stream, line);
		assert(std::regex_search(line.c_str(), match, PATTERN_COMMON));
		auto pointBasis = math::Vector2UInt({
			ui32(std::stoi(match[2].str())),
			ui32(std::stoi(match[1].str()))
		}).toFloat();

		getline(stream, line);
		assert(std::regex_search(line.c_str(), match, PATTERN_PAGE));
		auto atlasPath = fntPath.parent_path() / match[1].str();
		assert(std::filesystem::exists(atlasPath));
		BuildTexture::loadImage(atlasPath, atlasSize, atlasPixels);
		auto atlasSizeF = atlasSize.toFloat();

		getline(stream, line);
		assert(std::regex_search(line.c_str(), match, PATTERN_CHARCOUNT));
		uSize charCount = std::stoi(match[1].str());
		glyphs.resize(charCount);

		for (uIndex idxChar = 0; idxChar < charCount; ++idxChar)
		{
			getline(stream, line);
			assert(std::regex_search(line.c_str(), match, PATTERN_GLYPH));
			glyphs[idxChar] = asset::Font::Glyph {
				char(std::stoi(match[1].str())),
				
				// atlas pos
				math::Vector2({ f32(std::stoi(match[2].str())), f32(std::stoi(match[3].str())) }).toFloat() / atlasSizeF,
				// atlas size
				math::Vector2({ f32(std::stoi(match[4].str())), f32(std::stoi(match[5].str())) }).toFloat() / atlasSizeF,

				// the ratio of the basis for usage when rendering
				pointBasis.x() / pointBasis.y(),

				// size
				math::Vector2({ f32(std::stoi(match[4].str())), f32(std::stoi(match[5].str())) }) / pointBasis,
				// bearing/offset
				math::Vector2({ f32(std::stoi(match[6].str())), f32(std::stoi(match[7].str())) }) / pointBasis,
				// advance
				f32(std::stoi(match[8].str())) / pointBasis.x()
			
			};
		}
	}

	asset->setSDF(atlasSize, atlasPixels, glyphs);
	return errors;
}
