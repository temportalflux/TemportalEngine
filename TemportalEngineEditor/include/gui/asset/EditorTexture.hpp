#pragma once

#include "gui/asset/AssetEditor.hpp"

NS_GUI

namespace modal { class PathModal; }

class EditorTexture : public AssetEditor
{
public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	void setAsset(asset::AssetPtrStrong asset) override;
	void makeGui() override;

protected:
	void renderMenuBarItems() override;
	
private:
	std::weak_ptr<gui::modal::PathModal> mModalImport;

	void onImportConfirmed(std::filesystem::path path);

};

NS_END
