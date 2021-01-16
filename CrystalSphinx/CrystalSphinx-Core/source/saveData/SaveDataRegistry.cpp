#include "saveData/SaveDataRegistry.hpp"

using namespace saveData;

Instance::Instance(std::string name, std::filesystem::path root)
	: mName(name)
	, mRoot(root)
{
}

std::string const& Instance::name() const { return this->mName; }
std::filesystem::path Instance::userDirectory() const { return this->mRoot / "users"; }
std::filesystem::path Instance::chunkDirectory() const { return this->mRoot / "chunks"; }

void Instance::save()
{
	for (auto const& path : {
		this->mRoot,
		this->userDirectory(),
		this->chunkDirectory()
	})
	{
		if (!std::filesystem::exists(path))
			std::filesystem::create_directories(path);
	}
}

Registry::Registry(std::filesystem::path directory)
	: mDirectory(directory)
{
}

Instance& Registry::addEntry(std::string const& id, std::filesystem::path path)
{
	return this->mEntries.insert(std::make_pair(id, Instance(id, path))).first->second;
}

void Registry::scan()
{
	if (!std::filesystem::exists(this->mDirectory))
	{
		std::filesystem::create_directories(this->mDirectory);
	}
	if (std::filesystem::is_empty(this->mDirectory)) return;
	for (auto const& entry : std::filesystem::directory_iterator(this->mDirectory))
	{
		if (!entry.is_directory()) continue;
		auto name = entry.path().stem().string();
		this->addEntry(name, entry.path());
	}
}

bool Registry::has(std::string const& id) const
{
	return this->mEntries.find(id) != this->mEntries.end();
}

Instance& Registry::get(std::string const& id)
{
	auto iter = this->mEntries.find(id);
	assert(iter != this->mEntries.end());
	return iter->second;
}

Instance& Registry::create(std::string const& id)
{
	assert(!this->has(id));
	auto& instance = this->addEntry(id, this->mDirectory / id);
	instance.save();
	return instance;
}
