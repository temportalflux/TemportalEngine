#pragma once

#include "settings/SavedData.hpp"

NS_GAME

class ServerSettings
	: public SavedData
{
	static constexpr char const* defaultSaveId() { return "default"; }

public:
	ServerSettings()
		: SavedData("serverSettings.json")
		, mPort(25565)
		, mSaveId(defaultSaveId())
	{
	}

	ui16 port() const { return this->mPort; }
	std::string const& saveId() const { return this->mSaveId; }

private:

	ui16 mPort;
	std::string mSaveId;

protected:

	void save(cereal::JSONOutputArchive &archive) const override
	{
		archive(cereal::make_nvp("port", this->mPort));
		archive(cereal::make_nvp("saveId", this->mSaveId));
	}

	void load(cereal::JSONInputArchive &archive) override
	{
		archive(cereal::make_nvp("port", this->mPort));
		archive(cereal::make_nvp("saveId", this->mSaveId));
		if (this->mSaveId.length() == 0) this->mSaveId = defaultSaveId();
	}

};

NS_END
