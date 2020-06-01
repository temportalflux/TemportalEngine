
#include "Editor.hpp"
#include "Engine.hpp"
#include "memory/MemoryChunk.hpp"

int main()
{
	auto mainMemory = memory::MemoryChunk::Create(1 << 30);

	std::string logFileName = "TemportalEngine_Editor_" + logging::LogSystem::getCurrentTimeString() + ".log";
	engine::Engine::LOG_SYSTEM.open(logFileName.c_str());
	{
		auto editor = Editor(mainMemory);
		if (editor.setup())
		{
			editor.run();
		}
	}
	engine::Engine::LOG_SYSTEM.close();
	return 0;
}
