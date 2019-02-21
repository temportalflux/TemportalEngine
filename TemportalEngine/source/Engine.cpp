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
	mpWindowGame = new Window(640, 480, "Temportal Engine");
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
	LogEngineInfo("Received Input Event: %i", (i32)evt.type);

	if (evt.type == input::EInputType::QUIT)
		mpWindowGame->markShouldClose();

	if (evt.type == input::EInputType::KEY
		&& evt.inputKey.action == input::EAction::PRESS
		&& evt.inputKey.key == input::EKey::ESCAPE)
		mpWindowGame->markShouldClose();
}
