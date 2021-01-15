#pragma once

#include "CoreInclude.hpp"

#include "crypto/RSA.hpp"
#include "settings/UserInfo.hpp"
#include "utility/Guid.hpp"

NS_GAME

class UserIdRegistry
{

public:

	void setLimit(uSize userLimit);
	void scan(std::filesystem::path directory);

	utility::Guid const& createUser();
	utility::Guid const& addId(utility::Guid const& id);
	void initializeUser(utility::Guid const& id, crypto::RSAKey const& key);
	bool contains(utility::Guid const& id) const;
	uSize getUserCount() const;
	utility::Guid const& getId(uIndex idx) const;
	crypto::RSAKey loadKey(utility::Guid const& id) const;
	game::UserInfo loadInfo(utility::Guid const& id) const;

private:
	std::optional<uSize> mUserLimit;
	std::filesystem::path mDirectory;
	std::vector<utility::Guid> mUserIds;
	std::unordered_map<utility::Guid, std::filesystem::path> mUserDirById;

	std::filesystem::path getUserDir(utility::Guid const& id) const;

};

NS_END
