#include "world/Events.hpp"

TOnChunkLoadingListener WorldEventListener::onLoadingChunkEvent()
{
	return std::bind(&WorldEventListener::onLoadingChunk, this, std::placeholders::_1);
}

TOnChunkLoadingListener WorldEventListener::onUnloadingChunkEvent()
{
	return std::bind(&WorldEventListener::onUnloadingChunk, this, std::placeholders::_1);
}

TOnVoxelsChangedListener WorldEventListener::onVoxelsChangedEvent()
{
	return std::bind(&WorldEventListener::onVoxelsChanged, this, std::placeholders::_1);
}
