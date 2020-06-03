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

std::string IGui::getId() const
{
	return this->getTitle();
}

std::string IGui::getTitle() const
{
	return this->mTitle;
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
	if (this->beginView())
	{
		this->renderView();
	}
	this->endView();
}

bool IGui::beginView()
{
	auto labelAndId = this->getTitle() + "###" + this->getId();
	return ImGui::Begin(labelAndId.c_str(), &this->mbIsOpen, this->getFlags());
}

void IGui::endView()
{
	ImGui::End();
}
