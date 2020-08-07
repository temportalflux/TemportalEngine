#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "asset/TypedAssetPath.hpp"
#include "gui/graphics/PropertyGpuPreference.hpp"
#include "gui/widget/FieldNumber.hpp"
#include "gui/widget/FieldText.hpp"
#include "gui/widget/FieldAsset.hpp"

#include <version.h>

FORWARD_DEF(NS_ASSET, class RenderPass);

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

	asset::TypedAssetPath<asset::RenderPass> mSavedRenderPass;
	gui::FieldAsset mFieldRenderPass;

};

NS_END
