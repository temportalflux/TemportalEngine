#include "asset/AssetPath.hpp"

#include "Engine.hpp"

using namespace asset;

AssetPath::AssetPath(AssetType type, std::filesystem::path path, bool bIsAbsolute)
	: mType(type)
	, mPath(path)
	, mbIsAbsolute(bIsAbsolute)
{}

bool AssetPath::isValid() const
{
	return !this->mType.empty() && std::filesystem::exists(this->toAbsolutePath());
}

std::string AssetPath::toString() const
{
	return this->mType + ":" + this->mPath.string();
}

std::string AssetPath::toShortName() const
{
	return this->mType + ":" + this->mPath.stem().string();
}

std::string AssetPath::filename() const
{
	return this->mPath.stem().string();
}

std::string AssetPath::extension() const
{
	return this->mPath.extension().string();
}

std::filesystem::path AssetPath::toAbsolutePath() const
{
	if (this->mbIsAbsolute) return this->mPath;
	assert(engine::Engine::Get()->hasProject());
	return std::filesystem::absolute(engine::Engine::Get()->getProject()->getAbsoluteDirectoryPath() / this->mPath);
}
