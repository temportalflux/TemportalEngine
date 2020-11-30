#include "build/asset/BuildFont.hpp"

#include "asset/Font.hpp"
#include "math/Vector.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library g_FreeTypeLibrary;
bool g_FreeTypeInitialized = false;

using namespace build;

std::shared_ptr<BuildAsset> BuildFont::create(std::shared_ptr<asset::Asset> asset)
{
	return std::make_shared<BuildFont>(asset);
}

BuildFont::BuildFont(std::shared_ptr<asset::Asset> asset) : BuildAsset(asset)
{
}

std::vector<std::string> buildFontSize(FT_Face &face, ui8 size, math::Vector2UInt dpi, graphics::FontGlyphSet &glyphSet);
bool renderGlyph(FT_Face &face, char code, ui32 idxGlyph, graphics::FontGlyph &glyph, std::vector<std::string> &glyphErrors);

// See https://www.freetype.org/freetype2/docs/tutorial/step1.html
std::vector<std::string> BuildFont::compile(logging::Logger &logger)
{
	// Ensure that the freetype library is loaded
	if (!g_FreeTypeInitialized)
	{
		auto errorCode = FT_Init_FreeType(&g_FreeTypeLibrary);
		if (errorCode)
		{
			return { std::string("failed to initialize freetype library, error code:") + std::to_string(errorCode) };
		}
		g_FreeTypeInitialized = true;
	}

	auto asset = this->get<asset::Font>();

	// Load a font face file
	FT_Face fontFace;
	{
		auto errorCode = FT_New_Face(g_FreeTypeLibrary, asset->getFontPath().string().c_str(), 0, &fontFace);
		switch (errorCode)
		{
		case 0: break; // success
		case FT_Err_Unknown_File_Format:
			return { "unknown font file format" };
		default:
			return { "failed to load font source file" };
		}
	}

	std::vector<std::string> glyphErrors;

	this->mGlyphSets.clear();

	auto fontSizes = asset->getFontSizes();
	auto fontSizeCount = fontSizes.size();
	this->mGlyphSets.resize(fontSizeCount);
	for (uIndex i = 0; i < fontSizeCount; ++i)
	{
		auto errors = buildFontSize(fontFace, fontSizes[i], { 96, 96 }, this->mGlyphSets[i]);
		if (!errors.empty())
		{
			glyphErrors.insert(glyphErrors.end(), errors.begin(), errors.end());
		}
	}
	
	return glyphErrors;
}

std::vector<std::string> buildFontSize(FT_Face &face, ui8 size, math::Vector2UInt dpi, graphics::FontGlyphSet &glyphSet)
{
	// Set the character size
	{
		/*
			The character widths and heights are specified in 1/64th of points.
			A point is a physical distance, equaling 1/72th of an inch. Normally, it is not equivalent to a pixel.
			Value of 0 for the character width means ‘same as character height’, value of 0 for the character height
			means ‘same as character width’. Otherwise, it is possible to specify different character widths and heights.
			The horizontal and vertical device resolutions are expressed in dots-per-inch, or dpi. Standard values
			are 72 or 96 dpi for display devices like the screen. The resolution is used to compute the
			character pixel size from the character point size.
			Value of 0 for the horizontal resolution means ‘same as vertical resolution’, value of 0 for the
			vertical resolution means ‘same as horizontal resolution’.
			If both values are zero, 72 dpi is used for both dimensions.
		*/
		auto error = FT_Set_Char_Size(face, 0, size * 64, dpi.x(), dpi.y());
		if (error)
		{
			return {
				{ "failed to set character size " + std::to_string(size) + ". error code:" + std::to_string(error) }
			};
		}
	}

	std::vector<std::string> glyphErrors;

	ui32 idxGlyph, charCode;
	ui32 idxFontGlyph = 0;
	charCode = FT_Get_First_Char(face, &idxGlyph);
	graphics::FontGlyph glyph;
	while (idxGlyph != 0)
	{
		if (renderGlyph(face, (char)charCode, idxGlyph, glyph, glyphErrors))
		{
			glyphSet.glyphs.push_back(std::move(glyph));
			glyphSet.codeToGlyphIdx.insert(std::make_pair(charCode, idxFontGlyph++));
		}
		charCode = FT_Get_Next_Char(face, charCode, &idxGlyph);
	}

	return { glyphErrors };
}

template <typename T, typename U>
T getMetric(U& metric)
{
	return static_cast<T>(metric) / 64;
}

bool renderGlyph(FT_Face &face, char code, ui32 idxGlyph, graphics::FontGlyph &glyph, std::vector<std::string> &glyphErrors)
{
	auto error = FT_Load_Glyph(face, idxGlyph, FT_LOAD_DEFAULT);
	if (error)
	{
		glyphErrors.push_back(std::string("failed to load glyph for char '") + code + "'. code:" + std::to_string(error));
		return false;
	}
	FT_GlyphSlot slot = face->glyph;
	error = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
	if (error)
	{
		glyphErrors.push_back(std::string("failed to render glyph bitmap for char '") + code + "'. code:" + std::to_string(error));
		return false;
	}

	if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY || slot->bitmap.num_grays != 256)
	{
		glyphErrors.push_back(std::string("unsupported pixel mode ") + std::to_string(slot->bitmap.pixel_mode));
		return false;
	}

	// See http://freetype.sourceforge.net/freetype2/docs/tutorial/step2.html for what each metric is
	glyph.size = { getMetric<ui32>(slot->metrics.width), getMetric<ui32>(slot->metrics.height) };
	glyph.bearing = { (i32)(slot->bitmap_left), -(i32)(slot->bitmap_top) };
	glyph.advance = getMetric<ui32>(slot->advance.x);
	glyph.bufferSize = { slot->bitmap.width, slot->bitmap.rows };

	ui32 bufferSize = glyph.bufferSize.x() * glyph.bufferSize.y(); // a single channel to record only the alpha value (should be used with white color when loaded)
	if (bufferSize == 0)
	{
		// the glyph has nothing to render, but may just be empty space
		return true;
	}

	glyph.buffer.resize(bufferSize);

	ui8* src = slot->bitmap.buffer;
	ui8* rowStart = src;
	uIndex idxBuffer = 0;
	for (ui32 y = 0; y < glyph.bufferSize.y(); ++y)
	{
		src = rowStart;
		for (ui32 x = 0; x < glyph.bufferSize.x(); ++x)
		{
			glyph.buffer[idxBuffer++] = *src;
			src++;
		}
		rowStart += slot->bitmap.pitch;
	}

	return true;
}

void BuildFont::save()
{
	auto asset = this->get<asset::Font>();
	asset->glyphSets() = std::move(this->mGlyphSets);
	BuildAsset::save();
}
