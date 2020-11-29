#pragma once

#include "gui/asset/AssetEditor.hpp"

FORWARD_DEF(NS_GRAPHICS, class ImGuiTexture);

NS_GUI

namespace modal { class PathModal; }

class EditorTexture : public AssetEditor
{
public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	void setAsset(asset::AssetPtrStrong asset) override;
	void renderContent() override;

	void open() override;
	void close() override;

protected:
	void renderMenuBarItems() override;
	
private:
	std::shared_ptr<graphics::ImGuiTexture> mpTextureView;
	math::Vector2UInt mTextureSize;

	std::weak_ptr<gui::modal::PathModal> mModalImport;

	void onImportConfirmed(std::filesystem::path path);
	void loadPreview();

};

NS_END
