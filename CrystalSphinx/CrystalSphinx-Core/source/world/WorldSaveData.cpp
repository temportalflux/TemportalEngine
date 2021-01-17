#include "world/WorldSaveData.hpp"

using namespace world;

SaveData::SaveData() : JsonData()
{
}

SaveData::SaveData(std::filesystem::path filePath)
	: JsonData(filePath)
	, mSeed((ui32)time(0))
{
}

SaveData& SaveData::setSeed(ui32 seed)
{
	this->mSeed = seed;
	return *this;
}

ui32 const& SaveData::seed() const { return this->mSeed; }

void SaveData::save(cereal::JSONOutputArchive &archive) const
{
	archive(cereal::make_nvp("seed", this->mSeed));
}

void SaveData::load(cereal::JSONInputArchive &archive)
{
	archive(cereal::make_nvp("seed", this->mSeed));
}
