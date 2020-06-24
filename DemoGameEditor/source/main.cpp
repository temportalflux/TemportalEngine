#include "Editor.hpp"
#include "Engine.hpp"

int main(int argc, char *argv[])
{
	engine::Engine::startLogSystem("DemoGame-Editor");
	{
		auto editor = Editor(argc, argv);
		if (editor.setup())
		{
			editor.run();
		}
	}
	engine::Engine::stopLogSystem();
	return 0;
}
