#pragma once

#include "network/NetworkPacket.hpp"

#include "ecs/types.h"

NS_NETWORK
NS_PACKET

class DisplacePlayer : public Packet
{
	DECLARE_PACKET_TYPE(DisplacePlayer)

public:
	DisplacePlayer();

	DisplacePlayer& setTransformNetId(ecs::Identifier const& objectNetId);
	DisplacePlayer& move(math::Vector3 const& displacement, f32 deltaTime);

	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	void process(Interface *pInterface) override;

	static void moveController(
		network::Identifier ownerNetId,
		ecs::Identifier transformNetId,
		math::Vector3 const& displacement,
		f32 deltaTime
	);

private:
	ecs::Identifier mTransformObjectNetId;
	math::Vector3 mDisplacement;
	f32 mDeltaTime;

};

NS_END
NS_END
