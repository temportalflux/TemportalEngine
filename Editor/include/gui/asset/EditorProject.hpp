#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "gui/graphics/PropertyGpuPreference.hpp"
#include "gui/widget/FieldNumber.hpp"
#include "gui/widget/FieldText.hpp"
#include "gui/widget/List.hpp"

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
	
	// General
	gui::FieldText<32> mFieldName;
	gui::FieldNumber<ui8, 3> mFieldVersion;
	// Graphics
	gui::PropertyGpuPreference mGpuPreference;

	bool renderIntegerTestItem(ui32 &item);
	bool renderMapTestKey(std::pair<std::string, std::optional<ui32>> &item);
	bool renderMapTestValue(std::pair<std::string, std::optional<ui32>> &item);

};

NS_END
