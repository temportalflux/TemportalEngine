#include "settings/ServerSettings.hpp"

using namespace game;

constexpr char const* ServerSettings::defaultSaveId() { return "default"; }

ServerSettings::ServerSettings()
	: JsonData("serverSettings.json")
	, mPort(25565)
	, mSaveId(defaultSaveId())
{
}

ui16 ServerSettings::port() const { return this->mPort; }
std::string const& ServerSettings::saveId() const { return this->mSaveId; }

void ServerSettings::save(cereal::JSONOutputArchive &archive) const
{
	archive(cereal::make_nvp("port", this->mPort));
	archive(cereal::make_nvp("saveId", this->mSaveId));
}

void ServerSettings::load(cereal::JSONInputArchive &archive)
{
	archive(cereal::make_nvp("port", this->mPort));
	archive(cereal::make_nvp("saveId", this->mSaveId));
	if (this->mSaveId.length() == 0) this->mSaveId = defaultSaveId();
}
