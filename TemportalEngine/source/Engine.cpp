#include "Engine.hpp"
#include "Window.hpp"
#include "math/Vector.hpp"
#include "memory/MemoryManager.h"
#include <string>
#include "input/Queue.hpp"
#include "network/client/ServiceClient.hpp"
#include "network/server/ServiceServer.hpp"

using namespace engine;

logging::LogSystem Engine::LOG_SYSTEM = logging::LogSystem();
void* Engine::spInstance = nullptr;

void windowKeyInputCallback(input::Event const &inputEvt);
void inputQueueListener(input::Event const & evt);

constexpr uSize Engine::getMaxMemorySize()
{
	return 1 << 22; // 2^22
}

Engine* Engine::Create()
{
	if (spInstance == nullptr)
	{
		void* memoryManager = malloc(getMaxMemorySize());
		if (a3_mem_manager_init(memoryManager, getMaxMemorySize()))
		{
			if (a3_mem_manager_alloc(memoryManager, sizeof(Engine), &spInstance))
			{
				new (spInstance) Engine(TE_MAKE_VERSION(1, 0, 0), memoryManager);
				return Engine::Get();
			}
		}
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
		Engine *pEngine = static_cast<Engine*>(spInstance);
		void* memoryManager = pEngine->getMemoryManager();
		pEngine->~Engine();
		a3_mem_manager_dealloc(memoryManager, pEngine);
		spInstance = nullptr;
		free(memoryManager);
	}
}

Engine::Engine(ui32 const & version, void* memoryManager)
	: mpMemoryManager(memoryManager)
	, mIsRunning(false)
	, mpThreadRender(nullptr)
	, mpNetworkService(nullptr)
{
	LogEngineInfo("Creating Engine");
	mInfo.title = "TemportalEngine";
	mInfo.version = version;

	mpInputWatcher->setCallback(&windowKeyInputCallback);
	mpInputQueue = this->alloc<input::Queue>();
	mInputHandle = mpInputQueue->addListener(input::EInputType::QUIT,
		std::bind(&Engine::processInput, this, std::placeholders::_1)
	);
}

Engine::~Engine()
{
	this->terminateDependencies();
	this->dealloc(&mpInputQueue);
	LogEngineInfo("Engine Destroyed");
}

utility::SExecutableInfo const *const Engine::getInfo() const
{
	return &mInfo;
}

void * Engine::getMemoryManager()
{
	return mpMemoryManager;
}

void* Engine::allocRaw(uSize size)
{
	void* ptr = nullptr;
	this->mpLockMemoryManager->lock();
	a3_mem_manager_alloc(getMemoryManager(), size, &ptr);
	this->mpLockMemoryManager->unlock();
	return ptr;
}

void Engine::deallocRaw(void** ptr)
{
	this->mpLockMemoryManager->lock();
	a3_mem_manager_dealloc(getMemoryManager(), *ptr);
	this->mpLockMemoryManager->unlock();
	ptr = nullptr;
}

bool Engine::initializeDependencies()
{
	*mpDepSDL = dependency::SDL();
	if (!mpDepSDL->initialize()) return false;

	return true;
}

void Engine::terminateDependencies()
{
	if (mpDepSDL->isInitialized())
		mpDepSDL->terminate();
}

Window* Engine::createWindow(utility::SExecutableInfo const *const pAppInfo)
{
	auto window = this->alloc<Window>(800, 600, pAppInfo);
	window->addInputListeners(mpInputQueue);
	return window;
}

void Engine::createClient(char const *address, ui16 port)
{
	LogEngine(logging::ECategory::LOGINFO, "Initializing network client");
	auto client = this->alloc<network::ServiceClient>();
	client->initialize();
	client->connectToServer(address, port);
	this->mpNetworkService = client;
}

void Engine::createServer(ui16 const port, ui16 maxClients)
{
	LogEngine(logging::ECategory::LOGINFO, "Initializing network server");
	auto server = this->alloc<network::ServiceServer>();
	server->initialize(port, maxClients);
	this->mpNetworkService = server;
}

void Engine::run(Window* pWindow)
{
	mIsRunning = true;

	if (pWindow != nullptr && pWindow->isValid())
	{
		mpThreadRender = this->alloc<Thread>("Thread-Render", &Engine::LOG_SYSTEM);
		mpThreadRender->start(
			std::bind(&Window::renderUntilClose, pWindow),
			std::bind(&Window::waitForCleanup, pWindow)
		);
	}

	if (this->hasNetwork())
	{
		this->mpNetworkService->startThread(this);
	}

	while (this->isActive())
	{
		this->pollInput();
		mpInputQueue->dispatchAll();
	}

	if (this->hasNetwork())
		this->mpNetworkService->joinThread();
	
	mpThreadRender->join();
}

bool const Engine::isActive() const
{
	return mIsRunning;
}

void Engine::markShouldStop()
{
	mIsRunning = false;
}

bool const Engine::hasNetwork() const
{
	return this->mpNetworkService != nullptr;
}

std::optional<network::Service* const> Engine::getNetworkService() const
{
	if (this->hasNetwork())
		return this->mpNetworkService;
	return std::nullopt;
}

void Engine::pollInput()
{
	mpInputWatcher->pollInput();
}

void windowKeyInputCallback(input::Event const &inputEvt)
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
			LogEngineDebug("Mouse %i Press (%i) at (%i, %i)", (i32)evt.inputMouseButton.button, evt.inputMouseButton.clickCount, evt.inputMouseButton.coord[0], evt.inputMouseButton.coord[1]);
		if (evt.inputKey.action == input::EAction::RELEASE)
			LogEngineDebug("Mouse %i Release (%i) at (%i, %i)", (i32)evt.inputMouseButton.button, evt.inputMouseButton.clickCount, evt.inputMouseButton.coord[0], evt.inputMouseButton.coord[1]);
	}
	else if (evt.type == input::EInputType::MOUSE_SCROLL)
	{
		LogEngineDebug("Scroll by (%i, %i)", evt.inputScroll.delta[0], evt.inputScroll.delta[1]);
	}

	if (evt.type == input::EInputType::QUIT)
	{
		this->markShouldStop();
	}

	if (evt.type == input::EInputType::KEY
		&& evt.inputKey.action == input::EAction::PRESS
		&& evt.inputKey.key == input::EKey::ESCAPE)
	{
		this->markShouldStop();
	}

}
