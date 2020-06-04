#include "gui/asset/EditorShader.hpp"

#include "asset/Shader.hpp"
#include "gui/TextEditor.h"
#include "logging/Logger.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui;

#define Bit_ShaderContent 1 << 0

std::shared_ptr<AssetEditor> EditorShader::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	auto editor = mem->make_shared<EditorShader>();
	editor->mTextEditor = mem->make_shared<TextEditor>(); // automatically deallocated when the editor is
	return editor;
}

void EditorShader::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);
	
	this->mTextEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	this->mTextEditor->SetPalette(TextEditor::GetDarkPalette());
	

	auto asset = this->get<asset::Shader>();
	this->mSavedShaderContent = asset->readSource();
	this->mTextEditor->SetText(this->mSavedShaderContent);
}

void EditorShader::renderView()
{
	AssetEditor::renderView();

	auto asset = this->get<asset::Shader>();

	/* TODO: Can set error markers on lines in the editor
	TextEditor::ErrorMarkers markers;
	markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
	markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
	editor.SetErrorMarkers(markers)
	*/
	this->mTextEditor->Render("TextEditor");
	if (this->mTextEditor->IsTextChanged())
	{
		this->markAssetDirty(
			Bit_ShaderContent,
			this->mTextEditor->GetText() != this->mSavedShaderContent
		);
	}
}

void EditorShader::saveAsset()
{
	auto asset = this->get<asset::Shader>();
	
	if (this->isBitDirty(Bit_ShaderContent))
	{
		this->mSavedShaderContent = this->mTextEditor->GetText();
		asset->writeSource(this->mSavedShaderContent);

		// If the only change is the shader content, then only save that file
		if (this->getDirtyFlags() == Bit_ShaderContent)
		{
			this->markAssetClean();
			return;
		}
	}

	AssetEditor::saveAsset();
}
