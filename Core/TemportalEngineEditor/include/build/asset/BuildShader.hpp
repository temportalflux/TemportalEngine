#pragma once

#include "build/asset/BuildAsset.hpp"
#include "graphics/ShaderMetadata.hpp"

NS_BUILD

class BuildShader : public BuildAsset
{

public:
	static std::shared_ptr<BuildAsset> create(std::shared_ptr<asset::Asset> asset);

	BuildShader() = default;
	BuildShader(std::shared_ptr<asset::Asset> asset);

	void setContent(std::string content, ui32 stage);
	std::vector<std::string> compile(logging::Logger &logger) override;
	std::vector<ui32> getBinary() const;
	graphics::ShaderMetadata parseShader() const;
	void save() override;

private:
	std::string mSourceContent;
	ui32 mStage;
	std::vector<ui32> mCompiledBinary;

};

NS_END
