#pragma once

#include "gui/modal/Modal.hpp"

#include <array>

NS_GUI NS_MODAL

class ModalNewProject : public Modal
{
public:
	ModalNewProject();

protected:
	void drawContents() override;
	void reset() override;

private:
	std::array<char, 128> mInputDirectory;
	std::array<char, 32> mInputName;

	void submit();

};

NS_END NS_END
