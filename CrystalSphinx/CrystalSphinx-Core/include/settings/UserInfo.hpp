#pragma once

#include "settings/JsonData.hpp"

NS_GAME

class UserInfo : public JsonData
{
public:
	using Color = math::Vector<ui8, 3>;

	UserInfo();
	UserInfo(std::filesystem::path filePath);

	UserInfo& copyFrom(UserInfo const& info);
	UserInfo& setName(std::string const& name);
	std::string const& name() const;

	UserInfo& setColor(Color const& color);
	Color const& color() const;

protected:

	void save(cereal::JSONOutputArchive &archive) const override;
	void load(cereal::JSONInputArchive &archive) override;

private:
	// The name of the user. This can eventually pull from something like Steam integration.
	std::string mName;

	// not saved to data, only used when on game servers
	Color mColorOnConnectedServer;

};

NS_END
