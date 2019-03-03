#ifndef TE_ENGINE_HPP
#define TE_ENGINE_HPP

#include "Api.h"

#include "Namespace.h"
#include "dependency/SDL.hpp"
#include "thread/Thread.hpp"
#include "input/InputWatcher.hpp"
#include "network/NetworkService.hpp"
#include <optional>

#include "logging/Logger.hpp"

class Window;
namespace input
{
	class Queue;
}

NS_ENGINE

#define DeclareLog(title) logging::Logger(title, &engine::Engine::LOG_SYSTEM)
#define LogEngine(cate, ...) logging::Logger("Engine", &engine::Engine::LOG_SYSTEM).log(cate, __VA_ARGS__);
#define LogEngineInfo(...) LogEngine(logging::ECategory::LOGINFO, __VA_ARGS__)
#define LogEngineDebug(...) LogEngine(logging::ECategory::LOGDEBUG, __VA_ARGS__)

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

	network::Service *mpNetworkService;
	Thread *mpThreadRender;

	input::Queue *mpInputQueue;

	Engine(void* memoryManager);

public:
	~Engine();

	void* getMemoryManager();

	void* alloc(uSize size);
	void dealloc(void** ptr);

	template <typename TAlloc, typename... TArgs>
	TAlloc* alloc(TArgs... args)
	{
		TAlloc *ptr = (TAlloc*)this->alloc(sizeof(TAlloc));
		new (ptr) TAlloc(args...);
		return ptr;
	}

	template <typename TDealloc>
	void dealloc(TDealloc **ptrRef)
	{
		(*ptrRef)->TDealloc::~TDealloc();
		this->dealloc(ptrRef);
	}

	bool initializeDependencies();
	void terminateDependencies();

	bool createWindow();
	void destroyWindow();

	void createServer(ui16 const port, ui16 maxClients);
	void createClient(char const *address, ui16 port);
	void run();
	bool isActive() const;

	bool const hasNetwork() const;
	std::optional<network::Service* const> getNetworkService() const;

	void pollInput();
	void enqueueInput(struct input::Event const &evt);
	void processInput(struct input::Event const &evt);

};

NS_END

#endif