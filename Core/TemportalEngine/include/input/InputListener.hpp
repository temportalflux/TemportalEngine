#pragma once

#include "TemportalEnginePCH.hpp"

#include "input/Event.hpp"

NS_INPUT
class Queue;

class Listener : public virtual_enable_shared_from_this<Listener>
{

public:
	virtual ~Listener() {}

	std::shared_ptr<input::Queue> getQueue();

	void startListening(EInputType type);
	void stopListening(EInputType type);

protected:
	virtual void onInput(input::Event const& evt) = 0;

};

NS_END
