#pragma once

#include "TemportalEnginePCH.hpp"

FORWARD_DEF(NS_GRAPHICS, class UIRenderer);

NS_GRAPHICS

class UIString
{

public:
	
	static std::shared_ptr<UIString> create(std::string const& id, std::shared_ptr<graphics::UIRenderer> renderer);
	// Only public for std::memory allocation
	UIString(std::string const& id, std::weak_ptr<graphics::UIRenderer> renderer);

	void remove();

	std::string const& id() const;
	math::Vector2 const& position() const;
	std::string const& content() const;
	std::string const& fontId() const;
	ui8 const& fontSize() const;
	f32 pixelHeight() const;
	f32 thickness() const;
	f32 edgeDistance() const;

	UIString& setContent(std::string const& content);
	UIString& setPosition(math::Vector2 const& position);
	UIString& setFontId(std::string const& fontId);
	UIString& setFontSize(ui8 fontSize);

	UIString& setThickness(f32 width);
	UIString& setEdgeDistance(f32 distance);

	/**
	 * Updates the string in the renderer so it gets committed on the next frame.
	 * MUST be called after any of the mutation operations in order for the change to be rendered.
	 */
	UIString& update();

private:
	std::weak_ptr<graphics::UIRenderer> const mpRenderer;

	std::string const mId;

	// The value of the text. This is converted using `fontId` and `fontSize` into `vertices`.
	std::string mContent;

	// The screen-space ([0, 1]) position of the string,
	// where <0,0> is top-left and <1,1> is bottom-right.
	// The string's position is the top-left corner of the string. TODO: Add horizontal and vertical alignment
	math::Vector2 mPosition;

	// The identifier of the `RegisteredFont` in `UIRenderer`.
	std::string mFontId;
	ui8 mFontSize;

	f32 mWidth;
	f32 mEdgeDistance;

};

NS_END
