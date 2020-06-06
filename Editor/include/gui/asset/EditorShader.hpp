#pragma once

#include "gui/asset/AssetEditor.hpp"

#include <map>

FORWARD_DEF(NS_TASK, class TaskCompileShader)

NS_GUI
class TextEditor;

// Editor for `asset::Shader`
class EditorShader : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	void setAsset(asset::AssetPtrStrong asset) override;
	
	void makeGui() override;

protected:
	bool hasDetailsPanel() const { return true; }
	void renderDetailsPanel() override;
	void renderContent() override;
	void saveAsset() override;
	bool canCompileAsset() override;
	void compileAsset() override;

private:
	std::string mSavedShaderContent;
	std::shared_ptr<TextEditor> mTextEditor;

	std::shared_ptr<task::TaskCompileShader> mpCompilationTask;
	std::map<i32, std::string> mShaderCompilationErrors;

};

NS_END
