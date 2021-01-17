#pragma once

#include "CoreInclude.hpp"

#define NS_SAVE_DATA namespace saveData {

NS_SAVE_DATA

class Instance
{
public:
	Instance(std::string name, std::filesystem::path root);

	std::string const& name() const;
	std::filesystem::path userDirectory() const;
	std::filesystem::path worldSave() const;
	std::filesystem::path chunkDirectory() const;

	void save();

private:
	std::string mName;
	std::filesystem::path mRoot;

};

class Registry
{

public:
	Registry(std::filesystem::path directory);
	void scan();

	bool has(std::string const& id) const;
	Instance& get(std::string const& id);
	Instance& create(std::string const& id);

private:
	std::filesystem::path mDirectory;
	std::map<std::string, Instance> mEntries;

	Instance& addEntry(std::string const& id, std::filesystem::path path);

};

NS_END
