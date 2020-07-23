#include "gui/IGui.hpp"

#include "logging/Logger.hpp"
#include "Engine.hpp"
#include "graphics/ImGuiRenderer.hpp"

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
		// TODO: Allow these parameters to be passed in
		ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

		this->renderView();
	}
	this->endView();
	if (this->shouldReleaseGui())
	{
		this->removeGui();
	}
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

bool IGui::shouldReleaseGui() const
{
	return !this->isOpen();
}

void IGui::removeGui()
{
	this->mpOwner.lock()->removeGui(this->weak_from_this());
}
