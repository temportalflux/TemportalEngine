#ifndef TE_ENGINE_HPP
#define TE_ENGINE_HPP

#include "Namespace.h"
#include "dependency/GLFW.hpp"
#include "Window.h"
#include "thread/Thread.hpp"
#include "input/Queue.hpp"

#include "logging/Logger.hpp"

NS_ENGINE

#define DeclareLog(title) logging::Logger(title, &engine::Engine::LOG_SYSTEM)
#define LogEngine(cate, ...) logging::Logger("Engine", &engine::Engine::LOG_SYSTEM).log(cate, __VA_ARGS__);
#define LogEngineInfo(...) LogEngine(logging::ECategory::INFO, __VA_ARGS__)

class Engine
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

	GLFW mpDepGlfw[1];

	Window mpWindowGame[1];
	Thread<Window*> mpThreadRender[1];

	input::Queue mpInputQueue[1];

	Engine();

public:
	~Engine();

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