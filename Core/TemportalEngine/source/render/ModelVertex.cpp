#include "render/ModelVertex.hpp"

#include "cereal/mathVector.hpp"

bool ModelVertex::operator==(ModelVertex const& other) const
{
	return
		this->position == other.position
		&& this->texCoord == other.texCoord
		&& this->normal == other.normal
		&& this->tangent == other.tangent
		&& this->bitangent == other.bitangent;
}

void ModelVertex::save(cereal::PortableBinaryOutputArchive &archive) const
{
	archive(this->position);
	archive(this->texCoord);
	archive(this->normal);
	archive(this->tangent);
	archive(this->bitangent);
}

void ModelVertex::load(cereal::PortableBinaryInputArchive &archive)
{
	archive(this->position);
	archive(this->texCoord);
	archive(this->normal);
	archive(this->tangent);
	archive(this->bitangent);
}
