#pragma once

#include "network/NetworkPacket.hpp"

#include "ecs/types.h"
#include "world/WorldCoordinate.hpp"

NS_NETWORK
NS_PACKET

class DisplacePlayer : public Packet
{
	DECLARE_PACKET_TYPE(DisplacePlayer)

public:
	DisplacePlayer();

	DisplacePlayer& setTransformNetId(evcs::Identifier const& objectNetId);
	DisplacePlayer& moveTo(world::Coordinate const& position);

	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	void process(Interface *pInterface) override;

private:
	evcs::Identifier mTransformObjectNetId;
	world::Coordinate mPosition;

};

NS_END
NS_END
