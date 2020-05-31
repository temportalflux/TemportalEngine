#pragma once

#include "gui/modal/Modal.hpp"

#include <array>

NS_GUI NS_MODAL

class OpenAsset : public Modal
{
public:
	OpenAsset();

protected:
	void drawContents() override;
	void reset() override;

private:
	std::array<char, 128> mInputPath;

	void submit();

};

NS_END NS_END
