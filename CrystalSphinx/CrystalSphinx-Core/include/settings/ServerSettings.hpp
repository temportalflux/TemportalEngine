#pragma once

#include "settings/JsonData.hpp"

NS_GAME

class ServerSettings
	: public JsonData
{
	static constexpr char const* defaultSaveId();

public:
	ServerSettings();

	ui16 port() const;
	std::string const& saveId() const;

private:

	ui16 mPort;
	std::string mSaveId;

protected:

	void save(cereal::JSONOutputArchive &archive) const override;
	void load(cereal::JSONInputArchive &archive) override;

};

NS_END
