#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "gui/widget/FieldText.hpp"
#include "gui/widget/FieldNumber.hpp"

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
	Version mSavedVersion;
	
	gui::FieldText<32> mFieldName;
	gui::FieldNumber<ui8, 3> mFieldVersion;

};

NS_END
