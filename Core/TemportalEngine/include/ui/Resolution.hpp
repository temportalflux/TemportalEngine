#pragma once

#include "ui/Core.hpp"

NS_UI

struct Resolution
{
	
	math::Vector2UInt pixels;
	// DPI - https://docs.microsoft.com/en-us/windows/win32/learnwin32/dpi-and-device-independent-pixels
	ui32 dotsPerInch;

	math::Vector2 pointsToPixels(math::Vector2Int const& points) const
	{
		static const f32 INCHES_PER_POINT = (1.0f / 72.0f);
		/* Excerpt from https://docs.microsoft.com/en-us/windows/win32/learnwin32/dpi-and-device-independent-pixels
		When it comes to a computer display, however, measuring text size is problematic, because pixels are not all the same size.
		The size of a pixel depends on two factors: the display resolution, and the physical size of the monitor.
		Therefore, physical inches are not a useful measure, because there is no fixed relation between physical inches and pixels.
		Instead, fonts are measured in logical units. A 72-point font is defined to be one logical inch tall.
		Logical inches are then converted to pixels. For many years, Windows used the following conversion:
		One logical inch equals 96 pixels. Using this scaling factor, a 72-point font is rendered as 96 pixels tall.
		A 12-point font is 16 pixels tall.
		12 points = 12/72 logical inch = 1/6 logical inch = 96/6 pixels = 16 pixels
		*/
		return points.toFloat() * INCHES_PER_POINT * f32(this->dotsPerInch);
	}

	math::Vector2 pixelsToScreenSpace(math::Vector2 const& pixels) const
	{
		return pixels / this->pixels.toFloat();
	}

	math::Vector2 pointsToScreenSpace(math::Vector2Int const& points) const
	{
		return this->pixelsToScreenSpace(this->pointsToPixels(points));
	}

};

NS_END
