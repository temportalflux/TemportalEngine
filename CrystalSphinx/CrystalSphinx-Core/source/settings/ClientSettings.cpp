#include "settings/ClientSettings.hpp"

using namespace game;

ClientSettings::ClientSettings()
	: JsonData("clientSettings.json")
	, mResolution({ 1280, 720 })
	, mDpi(96)
{
}

math::Vector2UInt const& ClientSettings::resolution() const
{
	return this->mResolution;
}

ClientSettings& ClientSettings::setResolution(math::Vector2UInt const& resolution)
{
	this->mResolution = resolution;
	return *this;
}

ui32 const& ClientSettings::dpi() const { return this->mDpi; }

ClientSettings& ClientSettings::setDPI(ui32 dpi)
{
	this->mDpi = dpi;
	return *this;
}

void ClientSettings::save(cereal::JSONOutputArchive &archive) const
{
	archive(cereal::make_nvp("resolution", this->mResolution));
	archive(cereal::make_nvp("dpi", this->mDpi));
}

void ClientSettings::load(cereal::JSONInputArchive &archive)
{
	archive(cereal::make_nvp("resolution", this->mResolution));
	archive(cereal::make_nvp("dpi", this->mDpi));
}
