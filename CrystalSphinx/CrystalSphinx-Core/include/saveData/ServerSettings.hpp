#pragma once

#include "settings/JsonData.hpp"

NS_SAVE_DATA

class ServerSettings
	: public game::JsonData
{

public:
	ServerSettings();
	ServerSettings(std::filesystem::path parentDir);

	ui16 port() const;

private:

	ui16 mPort;

protected:

	void save(cereal::JSONOutputArchive &archive) const override;
	void load(cereal::JSONInputArchive &archive) override;

};

NS_END
