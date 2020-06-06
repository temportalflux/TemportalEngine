#pragma once

#include "gui/asset/AssetEditor.hpp"

#include <array>
#include <version.h>

NS_GUI

// Editor for `asset::Project`
class EditorProject : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	void setAsset(asset::AssetPtrStrong asset) override;

protected:
	void renderContent() override;
	void saveAsset() override;

private:
	// Input text for project name
	std::array<char, 32> mInputName;
	std::array<char, 3> mInputVersionMajor;
	std::array<char, 3> mInputVersionMinor;
	std::array<char, 4> mInputVersionPatch;

	std::string getInputName() const;
	Version getVersion() const;

};

NS_END
