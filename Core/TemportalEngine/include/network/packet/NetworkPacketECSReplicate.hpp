#pragma once

#include "network/NetworkPacket.hpp"

#include "evcs/types.h"

#define ECS_REPL_FIELD(CLASS_TYPE, FIELD_NAME) offsetof(CLASS_TYPE, FIELD_NAME), &FIELD_NAME, sizeof(FIELD_NAME)

FORWARD_DEF(NS_EVCS, class Core);
FORWARD_DEF(NS_EVCS, class IEVCSObject);
FORWARD_DEF(NS_EVCS, class NetworkedManager);

NS_NETWORK
NS_PACKET

class EVCSReplicate : public Packet
{
	DECLARE_PACKET_TYPE(EVCSReplicate)

public:
	enum class EReplicationType : i8
	{
		eInvalid = -1,
		eCreate,
		eUpdate,
		eDestroy,
	};

	EVCSReplicate();

	EVCSReplicate& setReplicationType(EReplicationType type);
	EVCSReplicate& setObjectEcsType(evcs::EType type);
	EVCSReplicate& setObjectTypeId(uIndex typeId);
	EVCSReplicate& setObjectNetId(evcs::Identifier const& netId);
	// Sets the owner for a given object.
	// If not provided, ownership is not replicated.
	// By default, objects are owned by the server.
	EVCSReplicate& setOwner(std::optional<ui32> ownerNetId);
	EReplicationType const& replicationType() const;
	evcs::EType const& ecsType() const;
	uIndex const& ecsTypeId() const;
	evcs::Identifier const& objectNetId() const;

	EVCSReplicate& pushLink(evcs::EType type, uIndex objectTypeId, evcs::Identifier netId);
	EVCSReplicate& pushComponentField(uIndex byteOffset, void* data, uSize dataSize);
	
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
	evcs::EType mObjectEcsType;

	/**
	 * The `ViewTypeId` or `ComponentTypeId` for a view or component being created.
	 * Not serialized for entities.
	 */
	uIndex mObjectTypeId;
	bool hasTypeId() const;

	/**
	 * The network identifier of the EVCS object.
	 * This is NOT the same as the evcs::Identifier in the receiver's instance,
	 * but rather the identifier the server has given the object
	 * (which maps to the identifier in the EVCS engine instance).
	 */
	evcs::Identifier mObjectNetId;

	bool mbHasOwnership;
	bool mbHasOwner;
	ui32 mOwnerNetId;

	struct ObjectLink
	{
		evcs::EType ecsType;
		evcs::TypeId objectTypeId;
		evcs::Identifier netId;
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

	void linkObject(evcs::NetworkedManager* manager, evcs::IEVCSObject* pObject);
	void fillFields(evcs::NetworkedManager* manager, evcs::IEVCSObject* pObject);
	std::optional<ui32> owner() const;

};

NS_END
NS_END
