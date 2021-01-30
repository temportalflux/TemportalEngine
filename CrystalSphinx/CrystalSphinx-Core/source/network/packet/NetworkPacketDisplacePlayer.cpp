#include "network/packet/NetworkPacketDisplacePlayer.hpp"

#include "Engine.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "game/GameInstance.hpp"
#include "network/NetworkInterface.hpp"
#include "world/World.hpp"

using namespace network;
using namespace network::packet;

DEFINE_PACKET_TYPE(DisplacePlayer)

DisplacePlayer::DisplacePlayer()
	: Packet(EPacketFlags::eReliable)
{
}

DisplacePlayer& DisplacePlayer::setTransformNetId(ecs::Identifier const& objectNetId)
{
	this->mTransformObjectNetId = objectNetId;
	return *this;
}

DisplacePlayer& DisplacePlayer::move(math::Vector3 const& displacement, f32 deltaTime)
{
	this->mDisplacement = displacement;
	this->mDeltaTime = deltaTime;
	return *this;
}

void DisplacePlayer::write(Buffer &archive) const
{
	Packet::write(archive);
	network::write(archive, "transformNetId", this->mTransformObjectNetId);
	network::write(archive, "displacement", this->mDisplacement);
	network::write(archive, "deltaTime", this->mDeltaTime);
}

void DisplacePlayer::read(Buffer &archive)
{
	Packet::read(archive);
	network::read(archive, "transformNetId", this->mTransformObjectNetId);
	network::read(archive, "displacement", this->mDisplacement);
	network::read(archive, "deltaTime", this->mDeltaTime);
}

void DisplacePlayer::process(Interface *pInterface)
{
	DisplacePlayer::moveController(
		pInterface->getNetIdFor(this->connection()),
		this->mTransformObjectNetId,
		this->mDisplacement, this->mDeltaTime
	);
}

void DisplacePlayer::moveController(
	network::Identifier ownerNetId,
	ecs::Identifier transformNetId,
	math::Vector3 const& displacement,
	f32 deltaTime
)
{
	auto pWorld = game::Game::Get()->world();
	auto& controller = pWorld->getPhysicsController(ownerNetId);
	controller.move(displacement, deltaTime);

	auto* ecs = ecs::Core::Get();
	if (game::Game::networkInterface()->type().includes(network::EType::eServer))
	{
		ecs->beginReplication();
	}
	auto* transform = dynamic_cast<ecs::component::CoordinateTransform*>(ecs->components().getObject(
		ecs::component::CoordinateTransform::TypeId,
		transformNetId
	));
	transform->setPosition(world::Coordinate::fromGlobal(controller.footPosition()));
	ecs->endReplication();
}
