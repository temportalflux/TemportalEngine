#pragma once

#include "settings/JsonData.hpp"

NS_GAME

class UserInfo : public JsonData
{
public:
	UserInfo();
	UserInfo(std::filesystem::path filePath);

	UserInfo& copyFrom(UserInfo const& info);
	UserInfo& setName(std::string const& name);
	std::string const& name() const;

protected:

	void save(cereal::JSONOutputArchive &archive) const override;
	void load(cereal::JSONInputArchive &archive) override;

private:
	// The name of the user. This can eventually pull from something like Steam integration.
	std::string mName;

};

NS_END
