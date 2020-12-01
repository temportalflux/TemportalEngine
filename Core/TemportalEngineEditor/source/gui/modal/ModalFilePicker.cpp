#include "gui/modal/ModalFilePicker.hpp"

#include <imgui.h>

using namespace gui::modal;

FilePicker::FilePicker(std::string titleId) : Modal(titleId)
{
}

void FilePicker::setRoot(std::filesystem::path const &path)
{
	this->mRoot = path;
}

void FilePicker::setPath(std::filesystem::path const &path)
{
	this->mCurrent = path;
	this->mFilePath.root = path;
	std::copy_n(path.string().begin(), path.string().length(), this->mFilePath.rawContent.begin());
}

void FilePicker::setConfig(gui::DirectoryViewConfig const &cfg)
{
	this->mViewCfg = cfg;
	this->mOnSubmit = this->mViewCfg.OnFileOpen;
	this->mViewCfg.OnFileOpen = std::bind(&FilePicker::onFileSelected, this, std::placeholders::_1);
}

void FilePicker::drawContents()
{
	ImGui::InputText("###filePath", this->mFilePath.rawContent.data(), this->mFilePath.rawContent.size());
	ImGui::SameLine();
	if (ImGui::Button("Open"))
	{
		this->submit();
	}

	// returns true if mCurrent is changed
	gui::renderFileDirectoryTree("DirectoryTree", this->mRoot, this->mCurrent, { 200, 0 });
	ImGui::SameLine();
	if (ImGui::BeginChild("BreadcrumbBrowser", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar))
	{
		gui::renderBreadcrumb(this->mRoot, this->mCurrent);
		gui::renderDirectoryView(this->mCurrent, this->mViewCfg);
	}
	ImGui::EndChild();
}

void FilePicker::onFileSelected(std::filesystem::path const &path)
{
	this->mFilePath.setPath(path);
}

void FilePicker::submit()
{
	this->close();
	if (this->mOnSubmit)
	{
		this->mOnSubmit(this->mFilePath.path());
	}
}
