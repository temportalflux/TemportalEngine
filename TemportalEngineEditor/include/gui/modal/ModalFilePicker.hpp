#pragma once

#include "gui/modal/Modal.hpp"

#include "gui/widget/filesystem.hpp"

NS_GUI NS_MODAL

class FilePicker : public Modal
{
public:
	FilePicker() = default;
	FilePicker(std::string titleId);

	void setRoot(std::filesystem::path const &path);
	void setConfig(gui::DirectoryViewConfig const &cfg);

protected:
	void drawContents() override;

private:
	std::filesystem::path mRoot, mCurrent;
	gui::PathText mFilePath;
	gui::DirectoryViewConfig mViewCfg;

	std::function<void(std::filesystem::path const &path)> mOnSubmit;

	void onFileSelected(std::filesystem::path const &path);
	void submit();

};

NS_END NS_END
