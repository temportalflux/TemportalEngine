#include "input/InputListener.hpp"

#include "Engine.hpp"
#include "input/Queue.hpp"

using namespace input;

std::shared_ptr<input::Queue> Listener::getQueue()
{
	return engine::Engine::Get()->getInputQueue();
}

void Listener::startListening(EInputType type)
{
	auto pQueue = this->getQueue();
	auto pThis = this->weak_from_this();
	assert(!pThis.expired());
	auto callback = std::bind(&Listener::onInput, this, std::placeholders::_1);
	pQueue->OnInputEvent.bind(type, pThis, callback);
}

void Listener::stopListening(EInputType type)
{
	auto pQueue = this->getQueue();
	auto pThis = this->weak_from_this();
	assert(!pThis.expired());
	pQueue->OnInputEvent.unbind(type, pThis);
}
