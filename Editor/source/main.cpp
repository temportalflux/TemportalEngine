
#include "Editor.hpp"
#include "Engine.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

int main(int argc, char *argv[])
{
	auto args = utility::parseArguments(argc, argv);

	uSize totalMem = 0;
	auto memoryChunkSizes = utility::parseArgumentInts(args, "memory-", totalMem);

	std::string logFileName = "TemportalEngine_Editor_" + logging::LogSystem::getCurrentTimeString() + ".log";
	engine::Engine::LOG_SYSTEM.open(logFileName.c_str());
	{
		auto editor = Editor(memoryChunkSizes);
		if (editor.setup(args))
		{
			editor.run(args);
		}
	}
	engine::Engine::LOG_SYSTEM.close();
	return 0;
}
