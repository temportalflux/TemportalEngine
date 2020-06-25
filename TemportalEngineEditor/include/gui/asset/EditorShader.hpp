#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "gui/widget/Combo.hpp"

#include <vulkan/vulkan.hpp>

FORWARD_DEF(NS_TASK, class TaskCompileShader)

NS_GUI
class TextEditor;

// Editor for `asset::Shader`
class EditorShader : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	void setAsset(asset::AssetPtrStrong asset) override;
	
protected:
	f32 getDetailsPanelWidth() const { return 200; }
	void renderDetailsPanel() override;
	void renderContent() override;
	void saveAsset() override;
	void onBuildFailure(std::vector<std::string> const &errors) override;

private:
	ui32 mSavedStage;
	std::string mSavedShaderContent;

	gui::Combo<vk::ShaderStageFlagBits> mComboStage;
	std::shared_ptr<TextEditor> mTextEditor;

	std::map<i32, std::string> mShaderCompilationErrors;

};

NS_END
