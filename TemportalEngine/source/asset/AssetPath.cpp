#include "asset/AssetPath.hpp"

#include "Engine.hpp"

using namespace asset;

AssetPath::AssetPath(AssetType type, std::filesystem::path path, bool bIsAbsolute)
	: mType(type)
	, mPath(path)
	, mbIsAbsolute(bIsAbsolute)
{}

std::string AssetPath::toString() const
{
	return this->mType + ":" + this->mPath.string();
}

std::string AssetPath::toShortName() const
{
	return this->mType + ":" + this->mPath.stem().string();
}

std::filesystem::path AssetPath::toAbsolutePath() const
{
	if (this->mbIsAbsolute) return this->mPath;
	assert(engine::Engine::Get()->hasProject());
	return std::filesystem::absolute(engine::Engine::Get()->getProject()->getAbsoluteDirectoryPath() / this->mPath);
}
