#pragma once

#include "settings/JsonData.hpp"

NS_GAME

class ClientSettings : public JsonData
{
public:
	ClientSettings();

	math::Vector2UInt const& resolution() const;
	ClientSettings& setResolution(math::Vector2UInt const& resolution);

	ui32 const& dpi() const;
	ClientSettings& setDPI(ui32 dpi);
	
protected:

	void save(cereal::JSONOutputArchive &archive) const override;
	void load(cereal::JSONInputArchive &archive) override;

private:
	math::Vector2UInt mResolution;
	ui32 mDpi;

};

NS_END
