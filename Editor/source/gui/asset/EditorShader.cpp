#include "gui/asset/EditorShader.hpp"

#include "Engine.hpp"
#include "TaskCompileShader.hpp"
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
	editor->mTextEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	editor->mTextEditor->SetPalette(TextEditor::GetDarkPalette());
	return editor;
}

void EditorShader::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);
	
	auto asset = this->get<asset::Shader>();
	this->mSavedShaderContent = asset->readSource();
	this->mTextEditor->SetText(this->mSavedShaderContent);
}

void EditorShader::makeGui()
{
	AssetEditor::makeGui();
	// while the ui may be closed, always ensure that if there is a compilation task running, it will get cleaned up
	if (this->mpCompilationTask)
	{
		if (this->mpCompilationTask->queryForCompletion())
		{
			auto asset = this->get<asset::Shader>();
			this->mShaderCompilationErrors.clear();
			// Make the error map by parsing all the errors return from the compilation task
			auto errors = this->mpCompilationTask->getErrors();
			this->mpCompilationTask.reset(); // reset the task so we can release the memory
			if (errors.size() > 0)
			{
				static std::regex RegexParseError(".*:([0-9]+): (.*)");
				std::smatch regexMatch;
				for (auto& err : errors)
				{
					if (std::regex_match(err, regexMatch, RegexParseError))
					{
						this->mShaderCompilationErrors.insert(std::make_pair(std::stoi(regexMatch[1].str()), regexMatch[2].str()));
					}
					else
					{
						this->mShaderCompilationErrors.insert(std::make_pair(0, err));
					}
				}
			}
		}
	}
}

void EditorShader::renderMenuBarItems()
{
	AssetEditor::renderMenuBarItems();
	if (ImGui::MenuItem("Compile", "", false, !this->mpCompilationTask))
	{
		// Always save current content when attempting to compile the shader
		this->saveAsset();
		auto asset = this->get<asset::Shader>();
		// pass in the source so we don't need to perform file read
		this->mpCompilationTask = engine::Engine::Get()->getMiscMemory()->make_shared<task::TaskCompileShader>(asset->getFileName());
		this->mpCompilationTask->compile(this->mSavedShaderContent, asset->getStage());
	}
}

void EditorShader::renderView()
{
	AssetEditor::renderView();

	this->mTextEditor->SetErrorMarkers(this->mShaderCompilationErrors);
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
		// the text editor ALWAYS appends an extra new line character, so lets remove it
		this->mSavedShaderContent = this->mSavedShaderContent.substr(0, this->mSavedShaderContent.length() - 1);
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
