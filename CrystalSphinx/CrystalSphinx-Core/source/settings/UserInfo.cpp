#include "settings/UserInfo.hpp"

using namespace game;

UserInfo::UserInfo() : JsonData() {}
UserInfo::UserInfo(std::filesystem::path filePath) : JsonData(filePath) {}

UserInfo& UserInfo::copyFrom(UserInfo const& info)
{
	this->mName = info.mName;
	this->mColorOnConnectedServer = info.mColorOnConnectedServer;
	return *this;
}

UserInfo& UserInfo::setName(std::string const& name)
{
	this->mName = name;
	return *this;
}

std::string const& UserInfo::name() const { return this->mName; }

UserInfo& UserInfo::setColor(Color const& color)
{
	this->mColorOnConnectedServer = color;
	return *this;
}

UserInfo::Color const& UserInfo::color() const { return this->mColorOnConnectedServer; }

void UserInfo::save(cereal::JSONOutputArchive &archive) const
{
	archive(cereal::make_nvp("name", this->mName));
	archive(cereal::make_nvp("color", this->mColorOnConnectedServer));
}

void UserInfo::load(cereal::JSONInputArchive &archive)
{
	archive(cereal::make_nvp("name", this->mName));
	archive(cereal::make_nvp("color", this->mColorOnConnectedServer));
}
