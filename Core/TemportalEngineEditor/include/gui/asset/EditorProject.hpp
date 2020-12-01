#pragma once

#include "gui/asset/AssetEditor.hpp"

NS_GUI

// Editor for `asset::Project`
class EditorProject : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

protected:
	void renderContent() override;

};

NS_END
