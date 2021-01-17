#pragma once

#include "settings/JsonData.hpp"

NS_WORLD

class SaveData : public game::JsonData
{

public:
	SaveData();
	SaveData(std::filesystem::path filePath);

	SaveData& setSeed(ui32 seed);
	ui32 const& seed() const;

protected:

	void save(cereal::JSONOutputArchive &archive) const override;
	void load(cereal::JSONInputArchive &archive) override;

private:
	ui32 mSeed;

};

NS_END
