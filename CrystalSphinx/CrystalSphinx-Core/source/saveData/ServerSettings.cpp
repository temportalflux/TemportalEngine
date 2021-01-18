#include "saveData/ServerSettings.hpp"

using namespace saveData;

ServerSettings::ServerSettings()
	: mPort(25565)
{
}

ServerSettings::ServerSettings(std::filesystem::path parentDir)
	: game::JsonData(parentDir / "serverSettings.json")
	, mPort(25565)
{
}

ui16 ServerSettings::port() const { return this->mPort; }

void ServerSettings::save(cereal::JSONOutputArchive &archive) const
{
	archive(cereal::make_nvp("port", this->mPort));
}

void ServerSettings::load(cereal::JSONInputArchive &archive)
{
	archive(cereal::make_nvp("port", this->mPort));
}
