#pragma once

#include "gui/asset/AssetEditor.hpp"

#include <array>

NS_GUI

// Editor for `asset::Settings`
class EditorSettings : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

protected:
	void setAsset(asset::AssetPtrStrong asset) override;
	void renderView() override;
	void saveAsset() override;

private:
	std::array<char, 256> mInput_OutputDirectory;

	std::string getOutputDirectory() const;

};

NS_END
