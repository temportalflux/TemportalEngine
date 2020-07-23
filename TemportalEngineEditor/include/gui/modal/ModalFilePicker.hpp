#pragma once

#include "gui/modal/Modal.hpp"

NS_GUI NS_MODAL

class FilePicker : public Modal
{
public:
	FilePicker() = default;
	FilePicker(char const *title);
	
protected:
	void drawContents() override;

private:
	void submit();

};

NS_END NS_END
