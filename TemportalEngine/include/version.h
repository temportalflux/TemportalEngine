#pragma once

#include "types/integer.h"

#include <string>
#include <cereal/cereal.hpp>

// Creates a unique 32-bit integer version for a unique semantic version
// NOTE: based on vulkan's VK_MAKE_VERSION
struct Version
{
	union
	{
		ui32 packed;
		struct
		{
			ui16 patch : 16;
			ui8 minor : 8;
			ui8 major : 8;
		} unpacked;
	};

	template<class Archive>
	void save(Archive& archive) const
	{
		archive(
			cereal::make_nvp("major", this->unpacked.major),
			cereal::make_nvp("minor", this->unpacked.minor),
			cereal::make_nvp("patch", this->unpacked.patch)
		);
	}

	template<class Archive>
	void load(Archive& archive)
	{
		ui8 major, minor;
		ui16 patch;
		archive(
			cereal::make_nvp("major", major),
			cereal::make_nvp("minor", minor),
			cereal::make_nvp("patch", patch)
		);
		this->unpacked = { patch, minor, major };
	}

	std::string toString() const
	{
		return
			"v" + std::to_string(this->unpacked.major)
			+ "." + std::to_string(this->unpacked.minor)
			+ "." + std::to_string(this->unpacked.patch);
	}
};

#define TE_MAKE_VERSION(major, minor, patch) (((major) << 24) | ((minor) << 16) | (patch))
#define TE_GET_MAJOR_VERSION(version) ((ui32)(version) >> 24)
#define TE_GET_MINOR_VERSION(version) (((ui32)(version) >> 16) & 0x3ff)
#define TE_GET_PATCH_VERSION(version) ((ui32)(version) & 0xfff)
#define TE_GET_VERSION(version) Version({ version })
