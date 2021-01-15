#pragma once

#include "settings/SavedData.hpp"

NS_GAME

class UserInfo : public SavedData
{
public:
	UserInfo() : SavedData() {}
	UserInfo(std::filesystem::path filePath) : SavedData(filePath) {}

	UserInfo& setName(std::string const& name) { this->mName = name; return *this; }
	std::string const& name() const { return this->mName; }

protected:

	void save(cereal::JSONOutputArchive &archive) const override
	{
		archive(cereal::make_nvp("name", this->mName));
	}

	void load(cereal::JSONInputArchive &archive) override
	{
		archive(cereal::make_nvp("name", this->mName));
	}

private:
	// The name of the user. This can eventually pull from something like Steam integration.
	std::string mName;

};

NS_END
