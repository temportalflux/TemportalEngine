#include "gui/IGui.hpp"

#include "logging/Logger.hpp"
#include "Engine.hpp"

#include <imgui.h>

using namespace gui;

IGui::IGui()
	: mbIsOpen(false)
{
}

IGui::IGui(std::string title) : IGui()
{
	this->mTitle = title;
}

logging::Logger* IGui::getLog() const
{
	static logging::Logger log = logging::Logger("Gui", &engine::Engine::LOG_SYSTEM);
	return &log;
}

bool IGui::isOpen() const
{
	return this->mbIsOpen;
}

void IGui::open()
{
	this->mbIsOpen = true;
}

void IGui::openOrFocus()
{
	if (!this->isOpen()) this->open();
	else ImGui::SetWindowFocus(this->mTitle.c_str());
}

void IGui::makeGui()
{
	if (!this->isOpen()) return;
	if (!this->beginView()) return;
	this->renderView();
	this->endView();
}

bool IGui::beginView()
{
	return ImGui::Begin(this->mTitle.c_str(), &this->mbIsOpen, this->getFlags());
}

void IGui::endView()
{
	ImGui::End();
}
