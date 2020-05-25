
#include "Editor.hpp"

int main()
{
	auto editor = Editor();
	editor.openWindow();
	editor.run();
	editor.closeWindow();
	return 0;
}
