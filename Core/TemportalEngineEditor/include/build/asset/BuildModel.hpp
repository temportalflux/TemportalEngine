#pragma once

#include "build/asset/BuildAsset.hpp"

#include "asset/ModelAsset.hpp"

NS_BUILD

class BuildModel : public BuildAsset
{

public:
	static std::shared_ptr<BuildAsset> create(std::shared_ptr<asset::Asset> asset);

	BuildModel() = default;
	BuildModel(std::shared_ptr<asset::Asset> asset);

	std::vector<std::string> compile(logging::Logger &logger) override;
	void save() override;

private:
	std::vector<asset::Model::Vertex> mCompiledVertices;
	std::vector<ui32> mCompiledIndices;

};

NS_END
