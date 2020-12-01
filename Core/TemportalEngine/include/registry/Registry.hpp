#ifndef TE_REGISTRY_HPP
#define TE_REGISTRY_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Engine ---------------------------------------------------------------------
#include "registry/RegistryItem.hpp"

// ----------------------------------------------------------------------------
#define RegisterType(pRegistry, type, value) pRegistry->registerType(type::Identification, value);
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
NS_UTILITY

template <class TValue, uSize TMaxCount>
class TEMPORTALENGINE_API Registry
{

private:

	uSize mItemCount;
	TValue maItems[TMaxCount];

public:
	Registry()
	{
		mItemCount = 0;
		memset(maItems, 0, sizeof(TValue) * TMaxCount);
	}

	bool registerType(RegistryIdentifier &id, TValue const &value)
	{
		if (id.has_value())
		{
			// TODO: Spit out error about duplicate registrations
			return false;
		}

		if (mItemCount + 1 >= TMaxCount)
		{
			// TODO: Throw error, too many items
			return false;
		}

		// TODO: Check if the hrti has been registered with anything before
		id = std::make_optional(mItemCount++);
		maItems[id.value()] = value;
		return true;
	}

	std::optional<TValue> operator[](RegistryIdentifier const & packetId) const
	{
		return at(packetId);
	}

	std::optional<TValue> at(RegistryIdentifier const & packetId) const
	{
		if (packetId.has_value() && packetId.value() < TMaxCount)
			return maItems[packetId.value()];
		else
			return std::nullopt;
	}

};

NS_END
// ----------------------------------------------------------------------------

#endif