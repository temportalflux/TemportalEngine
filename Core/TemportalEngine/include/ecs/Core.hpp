#pragma once

#include "TemportalEnginePCH.hpp"
#include "Delegate.hpp"

#include "ecs/types.h"
#include "ecs/entity/EntityManager.hpp"
#include "ecs/component/ComponentManager.hpp"
#include "ecs/view/ECViewManager.hpp"
#include "logging/Logger.hpp"

NS_NETWORK
FORWARD_DEF(NS_PACKET, class Packet);
FORWARD_DEF(NS_PACKET, class ECSReplicate);
NS_END

NS_ECS

class Core
{

public:

	static Core* Get();
	static logging::Logger& logger();

	Core();
	~Core();

	Core& setLog(logging::Logger log);

	EntityManager& entities();
	component::Manager& components();
	view::Manager& views();

	std::string typeName(ecs::EType type, ecs::TypeId typeId) const;
	std::string fullTypeName(ecs::EType type, ecs::TypeId typeId) const;

	BroadcastDelegate<void(ecs::EType, ecs::TypeId, ecs::IEVCSObject*)> onOwnershipChanged;

	template <typename... TArgs>
	void log(logging::ELogLevel category, logging::Message format, TArgs... args)
	{
		this->mLog.log(category, format, args...);
	}

	using ReplicationPacket = std::shared_ptr<network::packet::ECSReplicate>;
	/**
	 * Enables the auto-generation of replication packets.
	 * If called, `endReplication` MUST be called as soon
	 * as ecs modifications are complete.
	 */
	void beginReplication();
	/**
	 * Returns true if `beginReplication` was called
	 * but `endReplication` has not yet been called.
	 * Used primarily by internal structures to know if
	 * replication packets should be made.
	 */
	bool shouldReplicate() const;
	/**
	 * Creates an ecs replication packet and pushes it to
	 * the queue of packets to send when `endReplication` is called.
	 * Will assert if `shouldReplicate` returns false.
	 * Used internally to ECS to create packets.
	 */
	ReplicationPacket replicate();
	/**
	 * Creates an ecs replication packet to create an object.
	 * Uses `replicate` internally.
	 * Used internally to ECS to create packets.
	 */
	ReplicationPacket replicateCreate();
	/**
	 * Creates an ecs replication packet to update an object.
	 * Uses `replicate` internally.
	 * Used internally to ECS to create packets.
	 *
	 * If there is a create or update packet in the current
	 * replication queue with the same ecsType, typeId, and netId,
	 * then the packet is moved to the end of the queue is returned.
	 * Otherwise, a new packet is made with the update flag.
	 * This is an optimization so that the fewest possible packets are sent.
	 */
	ReplicationPacket replicateUpdate(
		ecs::EType ecsType,
		ecs::TypeId typeId,
		Identifier netId
	);
	/**
	 * Creates an ecs replication packet to destroy an object.
	 * Uses `replicate` internally.
	 * Used internally to ECS to create packets.
	 */
	ReplicationPacket replicateDestroy();
	/**
	 * If `beginReplication` is called, this function MUST be called
	 * once ECS operations are complete. If this is not called,
	 * data will never be replicated. If `beginReplication` is called
	 * and an additional `beginReplication` is called before this function
	 * is called, the application will encounter a fatal error.
	 * It is safe to call this function even if
	 * `beginReplication` has not been called.
	 * Broadcasts all queue packets to all connections, and clears the queue.
	 */
	void endReplication();

private:
	logging::Logger mLog;
	EntityManager mEntityManager;
	component::Manager mComponentManager;
	view::Manager mViewManager;

	bool mbIsReplicating;
	std::vector<std::shared_ptr<network::packet::Packet>> mReplicators;

};

NS_END
