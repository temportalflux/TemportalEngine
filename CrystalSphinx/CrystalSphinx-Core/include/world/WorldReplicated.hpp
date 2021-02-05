#pragma once

#include "world/World.hpp"

NS_NETWORK
FORWARD_DEF(NS_PACKET, class ChunkReplication);
NS_END

NS_WORLD

class WorldReplicated : public World
{

public:
	WorldReplicated();

	bool shouldConnectToPhysxDebugger() const override;
	void initializeDimension(network::packet::ChunkReplication *pPacket);

};

NS_END
