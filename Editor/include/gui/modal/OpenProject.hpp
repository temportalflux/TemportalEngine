#pragma once

#include "gui/modal/Modal.hpp"

#include <array>

NS_GUI NS_MODAL

class OpenProject : public Modal
{
public:
	OpenProject();

protected:
	void drawContents() override;
	void reset() override;

private:
	std::array<char, 128> mInputPath;

	void submit();

};

NS_END NS_END
