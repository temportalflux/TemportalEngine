#pragma once

#include "TemportalEnginePCH.hpp"

#define NS_MODAL namespace modal {

NS_GUI NS_MODAL

class Modal
{

public:
	Modal() = default;
	Modal(char const *title);

	void draw();
	virtual void open();
	void close();

protected:
	virtual void drawContents() = 0;
	virtual void reset();

private:
	char const *mTitle;
	bool mbShouldBeOpen;

};

NS_END NS_END
