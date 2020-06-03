#pragma once

#include "gui/asset/AssetEditor.hpp"

#include <array>

NS_GUI

// Editor for `asset::Project`
class EditorProject : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	void setAsset(asset::AssetPtrStrong asset) override;

protected:
	void renderView() override;
	void saveAsset() override;

private:
	// Input text for project name
	std::array<char, 32> mInputName;

	std::string getInputName() const;

};

NS_END
