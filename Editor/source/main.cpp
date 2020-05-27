
#include "Editor.hpp"

int main()
{
	auto editor = Editor();
	if (editor.setup())
	{
		editor.run();
	}
	return 0;
}
