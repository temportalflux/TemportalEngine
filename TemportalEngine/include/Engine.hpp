#ifndef TE_ENGINE_HPP
#define TE_ENGINE_HPP

#include "Api.h"

#include "Namespace.h"
#include "dependency/SDL.hpp"
#include "thread/Thread.hpp"
#include "input/Queue.hpp"
#include "input/InputWatcher.hpp"

#include "logging/Logger.hpp"

class Window;

NS_ENGINE

#define DeclareLog(title) logging::Logger(title, &engine::Engine::LOG_SYSTEM)
#define LogEngine(cate, ...) logging::Logger("Engine", &engine::Engine::LOG_SYSTEM).log(cate, __VA_ARGS__);
#define LogEngineInfo(...) LogEngine(logging::ECategory::INFO, __VA_ARGS__)
#define LogEngineDebug(...) LogEngine(logging::ECategory::DEBUG, __VA_ARGS__)

#define MAX_MEMORY_SIZE 2097152 // 8^7

class TEMPORTALENGINE_API Engine
{
private:
	static void* spInstance;

public:
	static logging::LogSystem LOG_SYSTEM;

	static Engine* Create();
	static Engine* Get();
	static bool GetChecked(Engine* &instance);
	static void Destroy();

private:

	TE_MutexLock mpLockMemoryManager[1];
	void* mpMemoryManager;

	SDL mpDepGlfw[1];

	Window *mpWindowGame;
	input::InputWatcher mpInputWatcher[1];

	Thread mpThreadRender[1];

	input::Queue mpInputQueue[1];

	Engine(void* memoryManager);

public:
	~Engine();

	void* getMemoryManager();
	void* alloc(uSize size);
	void dealloc(void** ptr);

	bool initializeDependencies();
	void terminateDependencies();

	bool createWindow();
	void destroyWindow();

	void run();

	void pollInput();
	void enqueueInput(struct input::Event const &evt);
	void processInput(struct input::Event const &evt);

};

NS_END

#endif