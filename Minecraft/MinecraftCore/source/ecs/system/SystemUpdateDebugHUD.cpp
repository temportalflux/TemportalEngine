#include "ecs/system/SystemUpdateDebugHUD.hpp"

#include "Window.hpp"
#include "ecs/view/ViewDebugHUD.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "render/ui/UIRenderer.hpp"
#include "render/ui/UIString.hpp"
#include "utility/StringUtils.hpp"

using namespace ecs;
using namespace ecs::system;

UpdateDebugHUD::UpdateDebugHUD(std::weak_ptr<Window> window)
	: System(view::DebugHUD::TypeId), mpWindow(window)
{
}

void UpdateDebugHUD::createHUD(std::shared_ptr<graphics::UIRenderer> renderer)
{
	auto testSize = graphics::UIString::create("debug:textTest", renderer)
		->setFontId("unispace").setFontSize(30)
		.setPosition({ 0.001f, 0.001f })
		.setContent("Sphinx of Black Quartz, Judge my vow")
		.update();

	(this->mUIStrings.transformPosition = graphics::UIString::create("debug:position", renderer))
		->setFontId("unispace").setFontSize(25)
		.setPosition({ 0.0f, 0.05f }).setContent("Position| X:<?,?,?> Y:<?,?,?> Z:<?,?,?>")
		.update();

	/*
	graphics::UIString::create("debug:cameraForwardLabel", renderer)
		->setFontId("unispace").setFontSize(20)
		.setPosition({ 0.f, 0.08f }).setContent("Forward:")
		.update();

	(this->mUIStrings.transformForward = graphics::UIString::create("debug:cameraForwardValue", renderer))
		->setFontId("unispace").setFontSize(20)
		.setPosition({ 0.12f, 0.08f }).setContent("<?,?,?>")
		.update();
	//*/

	(this->mUIStrings.fps = graphics::UIString::create("debug:fps", renderer))
		->setFontId("unispace").setFontSize(20)
		.setPosition({ 0.79f, 0.001f }).setContent("? fps")
		.update();
}

void UpdateDebugHUD::update(f32 deltaTime, std::shared_ptr<ecs::view::View> view)
{
	this->mOccurance = (this->mOccurance + 1) % 6000;
	if (this->mOccurance == 0)
	{
		this->updateOnOccurance(view);
	}
}

void UpdateDebugHUD::updateOnOccurance(std::shared_ptr<ecs::view::View> view)
{
	OPTICK_EVENT();
	auto transform = view->get<component::CoordinateTransform>();
	assert(transform);

	auto rot = transform->orientation().euler() * math::rad2deg();
	auto const& pos = transform->position();
	auto fwd = transform->forward();

	if (this->mUIStrings.transformPosition)
	{
		this->mUIStrings.transformPosition->setContent(utility::formatStr(
			"Position| X:<%i,%i,%.2f> Y:<%i,%i,%.2f> Z:<%i,%i,%.2f>",
			pos.chunk().x(), pos.local().x(), pos.offset().x(),
			pos.chunk().y(), pos.local().y(), pos.offset().y(),
			pos.chunk().z(), pos.local().z(), pos.offset().z()
		)).update();
	}
	if (this->mUIStrings.transformForward)
	{
		this->mUIStrings.transformForward->setContent(utility::formatStr("<%.2f, %.2f, %.2f>", fwd.x(), fwd.y(), fwd.z())).update();
	}
	if (this->mUIStrings.fps)
	{
		auto deltaMS = this->mpWindow.lock()->renderDurationMS();
		i32 fps = i32((1.0f / deltaMS) * 1000.0f);
		this->mUIStrings.fps->setContent(utility::formatStr("%i fps (%.2f ms)", fps, deltaMS)).update();
	}
}
