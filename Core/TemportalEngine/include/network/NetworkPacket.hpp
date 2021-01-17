#pragma once

#include "network/NetworkCore.hpp"

#include "network/data/NetworkDataBuffer.hpp"
#include "network/data/NetworkDataPrimitives.hpp"

#define DECLARE_PACKET_TYPE(ClassType) \
public: \
	static ui32 TypeId; \
	static std::shared_ptr<ClassType> create(); \
	ui32 typeId() const override { return ClassType::TypeId; } \
	std::string typeDisplayName() const override { return #ClassType; }
#define DEFINE_PACKET_TYPE(ClassType) \
	ui32 ClassType::TypeId = 0; \
	std::shared_ptr<ClassType> ClassType::create() { return std::make_shared<ClassType>(); }

NS_NETWORK
class Interface;

enum class EPacketFlags
{
	/**
	 * Send the message unreliably. Can be lost.
	 *
	 * Messages *can* be larger than a single MTU (UDP packet),
	 * but there is no retransmission, so if any piece of the message is lost,
	 * the entire message will be dropped.
	 *
	 * The sending API does have some knowledge of the underlying connection, so
	 * if there is no NAT-traversal accomplished or there is a recognized adjustment
	 * happening on the connection, the packet will be batched until the connection
	 * is open again.
	 */
	eUnreliable = 0,

	/**
	 * Disable Nagle's algorithm.
	 * 
	 * By default, Nagle's algorithm is applied to all outbound messages.  This means
	 * that the message will NOT be sent immediately, in case further messages are
	 * sent soon after you send this, which can be grouped together.  Any time there
	 * is enough buffered data to fill a packet, the packets will be pushed out immediately,
	 * but partially-full packets not be sent until the Nagle timer expires.  See
	 * ISteamNetworkingSockets::FlushMessagesOnConnection, ISteamNetworkingMessages::FlushMessagesToUser
	 * 
	 * NOTE: Don't just send every message without Nagle because you want packets to get there
	 * quicker.  Make sure you understand the problem that Nagle is solving before disabling it.
	 * If you are sending small messages, often many at the same time, then it is very likely that
	 * it will be more efficient to leave Nagle enabled.  A typical proper use of this flag is
	 * when you are sending what you know will be the last message sent for a while (e.g. the last
	 * in the server simulation tick to a particular client), and you use this flag to flush all
	 * messages.
	 * 
	 * If used in conjunction with `eUnreliable`, it is equivalent to using `eUnreliable`
	 * and then immediately flushing the messages using
	 * ISteamNetworkingSockets::FlushMessagesOnConnection or ISteamNetworkingMessages::FlushMessagesToUser.
	 * (But using this flag is more efficient since you only make one API call.)
	 */
	eNoNagle = 1,

	/**
	 * If the message cannot be sent very soon, then just drop it
	 * (because the connection is still doing some initial handshaking, route negotiations, etc).
	 * This is only applicable for unreliable messages.
	 * Using this flag on reliable messages is invalid.
	 *
	 * If used in conjunction with `eUnreliable` and `eNoNagle`:
	 * Send an unreliable message, but if it cannot be sent relatively quickly,
	 * then just drop it instead of queuing it.
	 * This is useful for messages that are not useful if they are excessively delayed, such as voice data.
	 * NOTE: The Nagle algorithm is not used, and if the message is not dropped,
	 * any messages waiting on the Nagle timer are immediately flushed.
	 * A message will be dropped under the following circumstances:
	 * - the connection is not fully connected.  (E.g. the "Connecting" or "FindingRoute" states)
	 * - there is a sufficiently large number of messages queued up already such that the current message
	 *   will not be placed on the wire in the next ~200ms or so.
	 * If a message is dropped for these reasons, k_EResultIgnored will be returned.
	 */
	eNoDelay = 4,

	/**
	 * Reliable message send. 
	 * 
	 * Can send up to k_cbMaxSteamNetworkingSocketsMessageSizeSend bytes in a single message.
	 *
	 * Does fragmentation/re-assembly of messages under the hood,
	 * as well as a sliding window for efficient sends of large chunks of data.
	 * 
	 * The Nagle algorithm is used. See notes on `eUnreliable` for more details.
	 */
	eReliable = 8,

	/** 
	 * By default, message sending is queued, and the work of encryption and talking to
	 * the operating system sockets, etc is done on a service thread.  This is usually a
	 * a performance win when messages are sent from the "main thread".  However, if this
	 * flag is set, and data is ready to be sent immediately (either from this message
	 * or earlier queued data), then that work will be done in the current thread, before
	 * the current call returns.  If data is not ready to be sent (due to rate limiting
	 * or Nagle), then this flag has no effect.
	 * 
	 * This is an advanced flag used to control performance at a very low level.  For
	 * most applications running on modern hardware with more than one CPU core, doing
	 * the work of sending on a service thread will yield the best performance.  Only
	 * use this flag if you have a really good reason and understand what you are doing.
	 * Otherwise you will probably just make performance worse.
	 */
	eUseCurrentThread = 16,

};

NS_PACKET

class Packet : public std::enable_shared_from_this<Packet>
{

public:
	// DEPRECATED
	struct Data {};
	
	Packet(utility::Flags<EPacketFlags> flags = 0);
	virtual ui32 typeId() const { assert(false); return 0; }
	virtual std::string typeDisplayName() const { assert(false); return ""; }

	utility::Flags<EPacketFlags> const& flags() const;

	void appendDebugLogHeader(std::stringstream &ss) const;
	virtual void write(Buffer &archive) const;
	virtual void read(Buffer &archive);
	
	void sendToServer();
	void send(ui32 connection);
	void sendTo(ui32 netId);
	void broadcast(std::set<ui32> except = {});

	void setSourceConnection(ui32 connection) { this->mConnection = connection; }
	ui32 connection() const { return this->mConnection; }
	virtual void process(Interface *pInterface) = 0;

private:
	utility::Flags<EPacketFlags> mFlags;
	ui32 mConnection;

};

NS_END
NS_END
