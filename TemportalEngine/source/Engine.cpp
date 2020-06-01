#include "Engine.hpp"

#include "memory/MemoryChunk.hpp"

#include "Window.hpp"
#include "math/Vector.hpp"
#include "memory/MemoryManager.h"
#include <string>
#include "input/Queue.hpp"
//#include "network/client/ServiceClient.hpp"
//#include "network/server/ServiceServer.hpp"

using namespace engine;

#define ENGINE_VERSION TE_MAKE_VERSION(1, 0, 0)

logging::LogSystem Engine::LOG_SYSTEM = logging::LogSystem();
Engine::EnginePtr Engine::spInstance = nullptr;

std::vector<const char*> Engine::VulkanValidationLayers = { "VK_LAYER_KHRONOS_validation" };

#pragma region Singleton

Engine::EnginePtr Engine::Create(std::shared_ptr<memory::MemoryChunk> pChunk)
{
	assert(Engine::spInstance == nullptr);
	Engine::spInstance = pChunk->make_shared<Engine>(pChunk);
	return Engine::Get();
}

Engine::EnginePtr Engine::Get()
{
	return Engine::spInstance;
}

void Engine::Destroy()
{
	Engine::spInstance = nullptr;
	assert(!Engine::spInstance);
}

#pragma endregion

Engine::Engine(std::shared_ptr<memory::MemoryChunk> mainMemory)
	: mMainMemory(mainMemory)
	, mbShouldContinueRunning(false)
	//, mpNetworkService(nullptr)
{
	LogEngineInfo("Creating Engine");
	mEngineInfo.title = "TemportalEngine";
	mEngineInfo.version = ENGINE_VERSION;

	this->mpInputWatcher[0] = input::InputWatcher(std::bind(
		&Engine::onRawEvent, this, std::placeholders::_1
	));
	this->mpInputWatcher->setInputEventCallback(std::bind(
		&Engine::enqueueInput, this, std::placeholders::_1
	));

	// TODO: allocate from a sub-chunk of main memory
	this->mpInputQueue = this->mMainMemory->make_shared<input::Queue>();
	this->mInputHandle = this->mpInputQueue->addListener(input::EInputType::QUIT,
		std::bind(&Engine::processInput, this, std::placeholders::_1)
	);

	this->mAssetManager.queryAssetTypes();
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

	this->mpInputQueue.reset();
	assert(!this->mpInputQueue);
	
	//this->mProject.reset();
	
	LogEngineInfo("Engine Destroyed");
}

bool Engine::hasProject() const
{
	return (bool)this->mProject;
}

void Engine::setProject(asset::ProjectPtrStrong project)
{
	this->mProject = project;
}

asset::ProjectPtrStrong Engine::getProject() const
{
	return this->mProject;
}

utility::SExecutableInfo const *const Engine::getInfo() const
{
	return &mEngineInfo;
}

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

std::shared_ptr<Window> Engine::createWindow(ui16 width, ui16 height, std::string title, WindowFlags flags)
{
	// TODO: allocate from a sub-chunk of main memory
	auto window = this->mMainMemory->make_shared<Window>(width, height, title, flags);
	window->addInputListeners(this->mpInputQueue);
	this->mWindowPtrs.insert(std::make_pair(window->getId(), window));
	return window;
}

void Engine::destroyWindow(std::shared_ptr<Window> &pWindow)
{
	this->mWindowPtrs.erase(pWindow->getId());
	pWindow->destroy();
}

#pragma endregion

#pragma region Graphics

bool Engine::setupVulkan()
{
	this->mVulkanInstance.setApplicationInfo(this->mProject->getName(), this->mProject->getVersion());
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

void Engine::start()
{
	this->mbShouldContinueRunning = true;

	for (auto [id, pWindow] : this->mWindowPtrs)
	{
		pWindow->startThread();
	}

	if (this->hasNetwork())
	{
		//this->mpNetworkService->startThread(this);
	}
}

bool const Engine::isActive() const
{
	return this->mbShouldContinueRunning;
}

void Engine::update()
{
	this->pollInput();
	mpInputQueue->dispatchAll();
	for (auto[id, pWindow] : this->mWindowPtrs)
	{
		pWindow->update();
	}
}

void Engine::joinThreads()
{
	//if (this->hasNetwork())
	//	this->mpNetworkService->joinThread();

	for (auto[id, pWindow] : this->mWindowPtrs)
	{
		pWindow->joinThread();
	}
}

void Engine::markShouldStop()
{
	this->mbShouldContinueRunning = false;
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
