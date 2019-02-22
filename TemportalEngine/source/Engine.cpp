#include "Engine.hpp"

using namespace engine;

logging::LogSystem Engine::LOG_SYSTEM = logging::LogSystem();
void* Engine::spInstance = nullptr;

void windowKeyInputCallback(Window* window, input::Event const &inputEvt);
void inputQueueListener(input::Event const & evt);

Engine* Engine::Create()
{
	if (spInstance == nullptr)
	{
		spInstance = new Engine();
		return Engine::Get();
	}
	return nullptr;
}

Engine * Engine::Get()
{
	return (Engine*)spInstance;
}

bool Engine::GetChecked(Engine *& instance)
{
	instance = Engine::Get();
	return instance != nullptr;
}

void Engine::Destroy()
{
	if (spInstance != nullptr)
	{
		delete (Engine*)spInstance;
		spInstance = nullptr;
	}
}

Engine::Engine()
{
	LogEngineInfo("Creating Engine");
	*mpInputQueue = input::Queue(&inputQueueListener);
}

Engine::~Engine()
{
	this->destroyWindow();
	this->terminateDependencies();
	LogEngineInfo("Engine Destroyed");
}

bool Engine::initializeDependencies()
{
	*mpDepGlfw = SDL();
	if (!mpDepGlfw->initialize()) return false;

	return true;
}

void Engine::terminateDependencies()
{
	if (mpDepGlfw->hasBeenInitialized())
		mpDepGlfw->terminate();
}

bool Engine::createWindow()
{
	mpWindowGame = new Window(800, 600, "Temportal Engine");
	if (!mpWindowGame->isValid()) return false;

	mpWindowGame->setInputCallback(&windowKeyInputCallback);
	mpWindowGame->initializeRenderContext(1);

	return true;
}

void Engine::destroyWindow()
{
	if (mpWindowGame->isValid())
	{
		mpWindowGame->destroy();
		delete mpWindowGame;
		mpWindowGame = nullptr;
	}
}

void Engine::run()
{
	*mpThreadRender = Thread("ThreadRender", &Engine::LOG_SYSTEM, &Window::renderUntilClose);
	mpThreadRender->start(mpWindowGame);

	while (mpWindowGame->isValid())
	{
		this->pollInput();
		mpInputQueue->dispatchAll();
	}
	mpThreadRender->join();

}

void Engine::pollInput()
{
	mpWindowGame->pollInput();
}

void windowKeyInputCallback(Window* window, input::Event const &inputEvt)
{
	Engine* pEngine;
	if (Engine::GetChecked(pEngine))
	{
		pEngine->enqueueInput(inputEvt);
	}
}

void inputQueueListener(input::Event const & evt)
{
	Engine* pEngine;
	if (Engine::GetChecked(pEngine))
	{
		pEngine->processInput(evt);
	}
}

void Engine::enqueueInput(input::Event const & evt)
{
	this->mpInputQueue->enqueue(evt);
}

void Engine::processInput(input::Event const & evt)
{
	//LogEngineDebug("Received Input Event| Type:%i", (i32)evt.type);
	if (evt.type == input::EInputType::KEY)
	{
		if (evt.inputKey.action == input::EAction::PRESS)
			LogEngineDebug("%i Press", (i32)evt.inputKey.key);
		if (evt.inputKey.action == input::EAction::REPEAT)
			LogEngineDebug("%i Repeat", (i32)evt.inputKey.key);
		if (evt.inputKey.action == input::EAction::RELEASE)
			LogEngineDebug("%i Release", (i32)evt.inputKey.key);
	}
	else if (evt.type == input::EInputType::MOUSE_MOVE)
	{
		//LogEngineDebug("MOVE by (%i, %i) to (%i, %i)", evt.inputMouseMove.xDelta, evt.inputMouseMove.yDelta, evt.inputMouseMove.xCoord, evt.inputMouseMove.yCoord);
	}
	else if (evt.type == input::EInputType::MOUSE_BUTTON)
	{
		if (evt.inputMouseButton.action == input::EAction::PRESS)
			LogEngineDebug("Mouse %i Press (%i) at (%i, %i)", (i32)evt.inputMouseButton.button, evt.inputMouseButton.clickCount, evt.inputMouseButton.xCoord, evt.inputMouseButton.yCoord);
		if (evt.inputKey.action == input::EAction::RELEASE)
			LogEngineDebug("Mouse %i Release (%i) at (%i, %i)", (i32)evt.inputMouseButton.button, evt.inputMouseButton.clickCount, evt.inputMouseButton.xCoord, evt.inputMouseButton.yCoord);
	}
	else if (evt.type == input::EInputType::MOUSE_SCROLL)
	{
		LogEngineDebug("Scroll by (%i, %i)", evt.inputScroll.xDelta, evt.inputScroll.yDelta);
	}

	if (evt.type == input::EInputType::QUIT)
		mpWindowGame->markShouldClose();

	if (evt.type == input::EInputType::KEY
		&& evt.inputKey.action == input::EAction::PRESS
		&& evt.inputKey.key == input::EKey::ESCAPE)
		mpWindowGame->markShouldClose();
}
