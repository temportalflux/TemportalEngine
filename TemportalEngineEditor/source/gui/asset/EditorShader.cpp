#include "gui/asset/EditorShader.hpp"

#include "Engine.hpp"
#include "Editor.hpp"
#include "asset/Shader.hpp"
#include "gui/TextEditor.h"
#include "logging/Logger.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include <imgui.h>

using namespace gui;

#define Bit_ShaderContent 1 << 0
#define Bit_Stage 1 << 1

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

	this->mComboStage
		.setOptions({
			vk::ShaderStageFlagBits::eVertex,
			vk::ShaderStageFlagBits::eTessellationControl,
			vk::ShaderStageFlagBits::eTessellationEvaluation,
			vk::ShaderStageFlagBits::eGeometry,
			vk::ShaderStageFlagBits::eFragment,
			vk::ShaderStageFlagBits::eCompute
		})
		.value((vk::ShaderStageFlagBits)(this->mSavedStage = asset->getStage()))
		.setCallbacks(
			[](vk::ShaderStageFlagBits flag) { return vk::to_string(flag); },
			[](vk::ShaderStageFlagBits flag) { ImGui::PushID((ui32)flag); }
		);

	this->mSavedShaderContent = asset->readSource();
	this->mTextEditor->SetText(this->mSavedShaderContent);
}

void EditorShader::renderDetailsPanel()
{
	auto asset = this->get<asset::Shader>();

	if (this->mComboStage.render("Stage"))
	{
		this->mSavedStage = (ui32)this->mComboStage.value();
		this->markAssetDirty(Bit_Stage, asset->getStage() != this->mSavedStage);
	}

	if (this->mSavedStage == (ui32)vk::ShaderStageFlagBits::eVertex)
	{
		if (ImGui::TreeNode("Generated Metadata"))
		{
			// TODO: Shader metadata should have its own type widget
			auto metadata = asset->getMetadata();
			if (!metadata.has_value())
			{
				ImGui::Text("No metadata. Please compile asset.");
			}
			else
			{
				if (ImGui::TreeNode("Input Attributes"))
				{
					for (auto& attrib : metadata.value().inputAttributes)
					{
						ImGui::Text((
							"Slot " + std::to_string(attrib.slot) + ": "
							+ attrib.typeName + " " + attrib.propertyName
						).c_str());
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
}

void EditorShader::renderContent()
{
	AssetEditor::renderContent();

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
	if (this->isBitDirty(Bit_Stage))
	{
		asset->setStage(this->mSavedStage);
	}

	AssetEditor::saveAsset();
}

void EditorShader::onBuildFailure(std::vector<std::string> const &errors)
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
