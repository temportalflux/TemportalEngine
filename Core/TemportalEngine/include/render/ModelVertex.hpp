#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

struct ModelVertex
{
	math::Vector3Padded position;

	math::Vector3Padded normal;
	math::Vector3Padded tangent;
	math::Vector3Padded bitangent;

	math::Vector2Padded texCoord;

	bool operator==(ModelVertex const& other) const;

	void save(cereal::PortableBinaryOutputArchive &archive) const;
	void load(cereal::PortableBinaryInputArchive &archive);
};
