#ifndef TE_REGISTRY_ITEM_HPP
#define TE_REGISTRY_ITEM_HPP

// Libraries ------------------------------------------------------------------
#include <optional>

// Engine ---------------------------------------------------------------------
#include "types/integer.h"

typedef std::optional<uSize> RegistryIdentifier;

// ----------------------------------------------------------------------------
#define GENERATE_IDENTIFICATION(humanReadableTypeIdentifier) \
private: \
enum { __GENERATED_TYPE_ID = humanReadableTypeIdentifier }; \
public: \
	static RegistryIdentifier Identification; \

// ----------------------------------------------------------------------------

#endif