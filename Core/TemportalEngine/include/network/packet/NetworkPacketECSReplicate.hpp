#pragma once

#include "network/NetworkPacket.hpp"

#include "ecs/types.h"

#define ECS_REPL_FIELD(CLASS_TYPE, FIELD_NAME) offsetof(CLASS_TYPE, FIELD_NAME), &FIELD_NAME, sizeof(FIELD_NAME)

FORWARD_DEF(NS_ECS, class IEVCSObject);
FORWARD_DEF(NS_ECS, class NetworkedManager);

NS_NETWORK
NS_PACKET

class ECSReplicate : public Packet
{
	DECLARE_PACKET_TYPE(ECSReplicate)

public:
	enum class EReplicationType : i8
	{
		eInvalid = -1,
		eCreate,
		eUpdate,
		eDestroy,
	};

	ECSReplicate();

	ECSReplicate& setReplicationType(EReplicationType type);
	ECSReplicate& setObjectEcsType(ecs::EType type);
	ECSReplicate& setObjectTypeId(uIndex typeId);
	ECSReplicate& setObjectNetId(ecs::Identifier const& netId);
	EReplicationType const& replicationType() const;
	ecs::EType const& ecsType() const;
	uIndex const& ecsTypeId() const;
	ecs::Identifier const& objectNetId() const;

	ECSReplicate& pushLink(ecs::EType type, uIndex objectTypeId, ecs::Identifier netId);
	ECSReplicate& pushComponentField(uIndex byteOffset, void* data, uSize dataSize);
	
	void write(Buffer &archive) const override;
	void read(Buffer &archive) override;
	void process(Interface *pInterface) override;

private:

	/**
	 * The event this packet represents.
	 * This indicates to the receiver how to process the event.
	 */
	EReplicationType mReplicationType;

	/**
	 * The kind of EVCS object being replicated.
	 */
	ecs::EType mObjectEcsType;

	/**
	 * The `ViewTypeId` or `ComponentTypeId` for a view or component being created.
	 * Not serialized for entities.
	 */
	uIndex mObjectTypeId;
	bool hasTypeId() const;

	/**
	 * The network identifier of the EVCS object.
	 * This is NOT the same as the ecs::Identifier in the receiver's instance,
	 * but rather the identifier the server has given the object
	 * (which maps to the identifier in the EVCS engine instance).
	 */
	ecs::Identifier mObjectNetId;

	struct ObjectLink
	{
		ecs::EType ecsType;
		ecs::TypeId objectTypeId;
		ecs::Identifier netId;
	};

	/**
	 * Links that are being added or destroyed.
	 * For entities, this could include views or components (determined by `ObjectLink#ecsType`).
	 * For views, these are components (so `ObjectLink#ecsType` must be `eComponent`).
	 */
	std::vector<ObjectLink> mObjectLinks;
	bool hasLinks() const;
	std::string toBufferString(std::vector<ObjectLink> const& links) const;
	void writeLinks(Buffer &archive) const;
	void readLinks(Buffer &archive);

	using ReplicatedData = std::vector<ui8>;
	using ReplicatedField = std::pair<uIndex, ReplicatedData>;
	std::vector<ReplicatedField> mComponentFields;
	bool hasFields() const;
	std::string toBufferString(std::vector<ReplicatedField> const& fields) const;
	void writeFields(Buffer &archive) const;
	void readFields(Buffer &archive);

	void linkObject(ecs::NetworkedManager* manager, ecs::IEVCSObject* pObject);

};

NS_END
NS_END
