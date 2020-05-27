#include "Engine.hpp"

#include "Window.hpp"
#include "math/Vector.hpp"
#include "memory/MemoryManager.h"
#include <string>
#include "input/Queue.hpp"
//#include "network/client/ServiceClient.hpp"
//#include "network/server/ServiceServer.hpp"

using namespace engine;

logging::LogSystem Engine::LOG_SYSTEM = logging::LogSystem();
void* Engine::spInstance = nullptr;

std::vector<const char*> Engine::VulkanValidationLayers = { "VK_LAYER_KHRONOS_validation" };

constexpr uSize Engine::getMaxMemorySize()
{
	return 1 << 22; // 2^22
}

#pragma region Singleton

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

#pragma endregion

Engine::Engine(ui32 const & version, void* memoryManager)
	: mpMemoryManager(memoryManager)
	, mIsRunning(false)
	, mpThreadRender(nullptr)
	//, mpNetworkService(nullptr)
{
	LogEngineInfo("Creating Engine");
	mEngineInfo.title = "TemportalEngine";
	mEngineInfo.version = version;

	this->mpInputWatcher[0] = input::InputWatcher(std::bind(
		&Engine::onRawEvent, this, std::placeholders::_1
	));
	this->mpInputWatcher->setInputEventCallback(std::bind(
		&Engine::enqueueInput, this, std::placeholders::_1
	));

	this->mpInputQueue = this->alloc<input::Queue>();
	this->mInputHandle = this->mpInputQueue->addListener(input::EInputType::QUIT,
		std::bind(&Engine::processInput, this, std::placeholders::_1)
	);
}

Engine::~Engine()
{
	// TODO: deallocate any windows that still exist in here
	this->mWindowPtrs.clear();

	if (this->mVulkanInstance.isValid())
	{
		this->mVulkanInstance.destroy();
	}
	this->terminateDependencies();
	this->dealloc(&mpInputQueue);
	LogEngineInfo("Engine Destroyed");
}

utility::SExecutableInfo const *const Engine::getInfo() const
{
	return &mEngineInfo;
}

void Engine::setApplicationInfo(utility::SExecutableInfo const *const pAppInfo)
{
	mAppInfo = *pAppInfo;
}

#pragma region Memory

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

#pragma endregion

#pragma region Dependencies

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

#pragma endregion

#pragma region Windows

Window* Engine::createWindow(ui16 width, ui16 height)
{
	auto window = this->alloc<Window>(width, height);
	window->addInputListeners(mpInputQueue);
	this->mWindowPtrs.insert(std::make_pair(window->getId(), window));
	return window;
}

void Engine::destroyWindow(Window* &pWindow)
{
	this->mWindowPtrs.erase(pWindow->getId());
	pWindow->destroy();
	engine::Engine::Get()->dealloc(&pWindow);
}

#pragma endregion

#pragma region Graphics

bool Engine::setupVulkan()
{
	this->mVulkanInstance.setApplicationInfo(mAppInfo);
	this->mVulkanInstance.setEngineInfo(mEngineInfo);
	this->mVulkanInstance.setValidationLayers(Engine::VulkanValidationLayers);
	this->mVulkanInstance.createLogger(&LOG_SYSTEM, /*vulkan debug*/ true);
	return true;
}

graphics::VulkanInstance* Engine::initializeVulkan(std::vector<const char*> requiredExtensions)
{
	this->mVulkanInstance.setRequiredExtensions(requiredExtensions);
	this->mVulkanInstance.initialize();
	return &this->mVulkanInstance;
}

#pragma endregion

#pragma region Game Loop

void Engine::run(Window* pWindow)
{
	mIsRunning = true;

	if (pWindow != nullptr && pWindow->isValid())
	{
		mpThreadRender = this->alloc<Thread>("Thread-Render", &Engine::LOG_SYSTEM);
		mpThreadRender->setFunctor(std::bind(&Window::renderUntilClose, pWindow));
		mpThreadRender->setOnComplete(std::bind(&Window::waitForCleanup, pWindow));
		mpThreadRender->start();
	}

	if (this->hasNetwork())
	{
		//this->mpNetworkService->startThread(this);
	}

	while (this->isActive())
	{
		this->pollInput();
		mpInputQueue->dispatchAll();
		pWindow->update();
	}

	//if (this->hasNetwork())
	//	this->mpNetworkService->joinThread();
	
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

#pragma endregion

#pragma region Input

void Engine::pollInput()
{
	mpInputWatcher->pollInput();
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

void Engine::onRawEvent(void *pEvt)
{
	for (auto [id, pWindow] : this->mWindowPtrs)
	{
		pWindow->onEvent(pEvt);
	}
}

#pragma endregion

void Engine::createClient(char const *address, ui16 port)
{
	LogEngine(logging::ECategory::LOGINFO, "Initializing network client");
	//auto client = this->alloc<network::ServiceClient>();
	//client->initialize();
	//client->connectToServer(address, port);
	//this->mpNetworkService = client;
}

void Engine::createServer(ui16 const port, ui16 maxClients)
{
	LogEngine(logging::ECategory::LOGINFO, "Initializing network server");
	//auto server = this->alloc<network::ServiceServer>();
	//server->initialize(port, maxClients);
	//this->mpNetworkService = server;
}

bool const Engine::hasNetwork() const
{
	return false;// this->mpNetworkService != nullptr;
}

/*
std::optional<network::Service* const> Engine::getNetworkService() const
{
	if (this->hasNetwork())
		return this->mpNetworkService;
	return std::nullopt;
}
//*/
