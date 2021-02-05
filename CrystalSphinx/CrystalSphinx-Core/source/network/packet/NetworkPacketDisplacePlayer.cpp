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

DisplacePlayer& DisplacePlayer::moveTo(world::Coordinate const& position)
{
	this->mPosition = position;
	return *this;
}

void DisplacePlayer::write(Buffer &archive) const
{
	Packet::write(archive);
	network::write(archive, "transformNetId", this->mTransformObjectNetId);
	network::write(archive, "position", this->mPosition);
}

void DisplacePlayer::read(Buffer &archive)
{
	Packet::read(archive);
	network::read(archive, "transformNetId", this->mTransformObjectNetId);
	network::read(archive, "position", this->mPosition);
}

void DisplacePlayer::process(Interface *pInterface)
{
	auto* ecs = ecs::Core::Get();
	auto* transform = dynamic_cast<ecs::component::CoordinateTransform*>(ecs->components().getObject(
		ecs::component::CoordinateTransform::TypeId,
		this->mTransformObjectNetId
	));

	auto const ownerNetId = pInterface->getNetIdFor(this->connection());
	auto pWorld = game::Game::Get()->world();
	auto& controller = pWorld->getPhysicsController(ownerNetId);

	auto const displacement = this->mPosition - transform->position();
	controller.move(displacement.toGlobal().toFloat(), pWorld->simulationFrequency());

	// the position of the transform as dictated by the server
	auto adjustedPosition = world::Coordinate::fromGlobal(controller.footPosition());
	// the difference in the position between the client's expectation and the server's dictation
	auto adjustedDiff = this->mPosition - adjustedPosition;

	#ifdef LOG_DEBUG_ENABLED
	this->logger().log(
		LOG_DEBUG,
		"Applying player displacement:\nFrom: %s\nTo: %s\nSqMag Diff: %0.5f\nAdjusted: %s\nDesired Diff: %s\nSqMag Diff: %0.5f",
		transform->position().toString().c_str(),
		this->mPosition.toString().c_str(),
		displacement.toGlobal().magnitudeSq(),
		adjustedPosition.toString().c_str(),
		adjustedDiff.toString().c_str(),
		adjustedDiff.toGlobal().magnitudeSq()
	);
	#endif

	auto ignoredConnections = std::set<network::ConnectionId>();
	// If the adjustedPosition is roughly equivalent to the client's prediction, don't replicate to them
	if (adjustedDiff.toGlobal().magnitudeSq() <= math::epsilon())
	{
		ignoredConnections.insert(this->connection());
	}

	if (game::Game::networkInterface()->type().includes(network::EType::eServer))
	{
		ecs->beginReplication();
	}
	transform->setPosition(adjustedPosition);
	ecs->endReplication(ignoredConnections);
}
