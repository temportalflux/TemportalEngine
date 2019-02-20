#ifndef TE_ENGINE_HPP
#define TE_ENGINE_HPP

#include "Namespace.h"
#include "dependency/GLFW.h"
#include "Window.h"
#include "input/Queue.hpp"
#include "thread/Thread.h"

NS_ENGINE

class Engine
{
private:
	static void* spInstance;

public:
	static char const * Log;

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

#define LogEngineInfo(...) logging::log(engine::Engine::Log, logging::ECategory::INFO, __VA_ARGS__);

NS_END

#endif