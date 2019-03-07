#include "network/common/PacketRegistry.hpp"

#include <type_traits>

// ----------------------------------------------------------------------------

using namespace network;

// ----------------------------------------------------------------------------

PacketRegistry::PacketRegistry()
	: Registry()
{
}

std::optional<Packet*> PacketRegistry::getPacketFrom(RegistryIdentifier const & id, void * data) const
{
	auto functor = this->at(id);
	if (functor.has_value() && functor.value() != nullptr)
	{
		return functor.value()(data);
	}
	else
	{
		return std::nullopt;
	}
}
