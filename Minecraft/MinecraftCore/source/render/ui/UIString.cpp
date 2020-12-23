#include "render/ui/UIString.hpp"

#include "render/ui/UIRenderer.hpp"

using namespace graphics;

std::shared_ptr<UIString> UIString::create(std::string const& id, std::shared_ptr<graphics::UIRenderer> renderer)
{
	auto handle = std::make_shared<UIString>(id, std::weak_ptr(renderer));
	renderer->addString(handle);
	return handle;
}

UIString::UIString(std::string const& id, std::weak_ptr<graphics::UIRenderer> renderer)
	: mId(id), mpRenderer(renderer)
	, mPosition({ 0, 0 }), mContent("")
	, mFontId(""), mFontSize(0)
{
}

void UIString::remove()
{
	if (!this->mpRenderer.expired())
	{
		this->mpRenderer.lock()->removeString(this);
	}
}

std::string const& UIString::id() const { return this->mId; }
math::Vector2 const& UIString::position() const { return this->mPosition; }
std::string const& UIString::content() const { return this->mContent; }
std::string const& UIString::fontId() const { return this->mFontId; }
ui8 const& UIString::fontSize() const { return this->mFontSize; }

f32 UIString::pixelHeight() const
{
	// rendering font at 72dpi (pixels per inch)
	// means the resolution is 1:1 with point size
	static const f32 dpi = 72;
	static const f32 POINTS_PER_INCH = (1.0f / 72.0f);
	ui8 pointSize = this->fontSize();
	// the height of the text in pixels
	return f32(pointSize) * POINTS_PER_INCH * dpi;
}

UIString& UIString::setContent(std::string const& content)
{
	this->mContent = content;
	return *this;
}

UIString& UIString::setPosition(math::Vector2 const& position)
{
	this->mPosition = position;
	return *this;
}

UIString& UIString::setFontId(std::string const& fontId)
{
	this->mFontId = fontId;
	return *this;
}

UIString& UIString::setFontSize(ui8 fontSize)
{
	this->mFontSize = fontSize;
	return *this;
}

UIString& UIString::update()
{
	this->mpRenderer.lock()->updateString(this);
	return *this;
}
