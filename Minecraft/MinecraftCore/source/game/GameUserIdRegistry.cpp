#include "game/GameUserIdRegistry.hpp"

#include "utility/TimeUtils.hpp"

#include <fstream>

using namespace game;

#define PRIVATE_KEY_FILE_NAME "private_key.pem"
#define PUBLIC_KEY_FILE_NAME "public_key.pem"
#define USER_DATA_FILE_NAME "user.json"

void UserIdRegistry::setLimit(uSize userLimit)
{
	this->mUserLimit = userLimit;
}

void UserIdRegistry::scan(std::filesystem::path directory)
{
	this->mDirectory = directory;
	
	this->mUserIds.clear();
	this->mUserDirById.clear();
	if (!std::filesystem::exists(directory))
	{
		std::filesystem::create_directories(directory);
		return;
	}
	
	for (auto const& entry : std::filesystem::directory_iterator(directory))
	{
		if (this->mUserLimit && this->getUserCount() >= *this->mUserLimit) break;
		if (entry.is_directory())
		{
			auto userDir = entry.path();
			auto id = utility::Guid::fromString(userDir.stem().filename().string());
			this->mUserIds.push_back(id);
			this->mUserDirById.insert(std::make_pair(id, userDir));
		}
	}
}

utility::Guid const& UserIdRegistry::createUser()
{
	auto const& id = this->addId(utility::Guid::create());
	auto key = crypto::RSAKey();
	key.generate();
	this->initializeUser(id, key);
	return id;
}

utility::Guid const& UserIdRegistry::addId(utility::Guid const& id)
{
	assert(!this->mUserLimit || this->getUserCount() < this->mUserLimit);
	auto iter = this->mUserDirById.insert(std::make_pair(id, this->mDirectory / id.toString()));
	return iter.first->first;
}

void UserIdRegistry::initializeUser(utility::Guid const& id, crypto::RSAKey const& key)
{
	auto userDir = this->getUserDir(id);
	
	// Save the user's public or private key (depending on what was provided)
	std::filesystem::create_directories(userDir);
	{
		auto bIsPrivate = key.isPrivate();
		auto keyPath = userDir / (bIsPrivate ? PRIVATE_KEY_FILE_NAME : PUBLIC_KEY_FILE_NAME);
		auto stream = std::ofstream(keyPath.string().c_str(), std::ios::trunc | std::ios::out);
		std::string keyString;
		if (bIsPrivate) keyString = key.privateKey().value();
		else keyString = key.publicKey().value();
		stream << keyString;
	}

	// Save the user's settings (i.e. name and game related information)
	game::UserInfo(userDir / USER_DATA_FILE_NAME).writeToDisk();
}

uSize UserIdRegistry::getUserCount() const
{
	return this->mUserIds.size();
}

utility::Guid const& UserIdRegistry::getId(uIndex idx) const
{
	return this->mUserIds[idx];
}

bool UserIdRegistry::contains(utility::Guid const& id) const
{
	return this->mUserDirById.find(id) != this->mUserDirById.end();
}

std::filesystem::path UserIdRegistry::getUserDir(utility::Guid const& id) const
{
	auto iter = this->mUserDirById.find(id);
	assert(iter != this->mUserDirById.end());
	return iter->second;
}

std::pair<std::ifstream, uIndex> openAvailable(std::vector<std::filesystem::path> paths)
{
	for (uIndex i = 0; i < paths.size(); ++i)
	{
		if (std::filesystem::exists(paths[i]))
		{
			return std::make_pair(
				std::ifstream(paths[i].string().c_str()), i
			);
		}
	}
	assert(false);
	return std::make_pair(std::ifstream(), 0);
}

crypto::RSAKey UserIdRegistry::loadKey(utility::Guid const& id) const
{
	auto userDir = this->getUserDir(id);
	bool bIsPrivate = false;
	std::string fileContent;
	{
		auto found = openAvailable({
			userDir / PRIVATE_KEY_FILE_NAME, userDir / PUBLIC_KEY_FILE_NAME
		});
		bIsPrivate = found.second == 0;
		fileContent.assign(
			std::istreambuf_iterator<char>(found.first),
			std::istreambuf_iterator<char>()
		);
	}
	crypto::RSAKey outKey;
	if (bIsPrivate && crypto::RSAKey::fromPrivateString(fileContent, outKey))
	{
		return outKey;
	}
	else if (crypto::RSAKey::fromPublicString(fileContent, outKey))
	{
		return outKey;
	}
	else
	{
		assert(false);
		return outKey;
	}
}

game::UserInfo UserIdRegistry::loadInfo(utility::Guid const& id) const
{
	auto settings = game::UserInfo(this->getUserDir(id) / USER_DATA_FILE_NAME);
	settings.readFromDisk();
	return settings;
}
