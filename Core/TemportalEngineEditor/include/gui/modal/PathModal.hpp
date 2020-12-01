#pragma once

#include "gui/modal/Modal.hpp"

NS_GUI NS_MODAL

class PathModal : public Modal
{
public:
	typedef std::function<void(std::filesystem::path path)> PathSelectedCallback;

	PathModal() = default;
	PathModal(std::string title);

	void setDefaultPath(std::filesystem::path path);
	void setCallback(PathSelectedCallback callback);

protected:
	void drawContents() override;

private:
	std::array<char, 128> mInputPath;
	PathSelectedCallback mOnPathSelected;

	void submit();

};

NS_END NS_END
