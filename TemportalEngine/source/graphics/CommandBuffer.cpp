#include "graphics/CommandBuffer.hpp"

using namespace graphics;

Command CommandBuffer::beginCommand()
{
	this->mInternal->begin(vk::CommandBufferBeginInfo());
	return Command(this);
}

void CommandBuffer::endCommand(Command *command)
{
	this->mInternal->end();
}
