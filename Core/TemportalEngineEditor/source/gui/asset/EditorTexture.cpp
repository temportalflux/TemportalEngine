#include "gui/asset/EditorTexture.hpp"

#include "asset/AssetManager.hpp"
#include "asset/Texture.hpp"
#include "gui/modal/PathModal.hpp"
#include "memory/MemoryChunk.hpp"
#include "Editor.hpp"
#include "graphics/ImGuiTexture.hpp"
#include "build/asset/BuildTexture.hpp"
#include "property/Property.hpp"

#include "math/Vector.hpp"

#include <imgui.h>

using namespace gui;

std::shared_ptr<AssetEditor> EditorTexture::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorTexture>();
}

void EditorTexture::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);
}

void EditorTexture::open()
{
	AssetEditor::open();
	this->mpTextureView = Editor::EDITOR->renderer()->reserveTexture();
	this->loadPreview();
}

void EditorTexture::close()
{
	AssetEditor::close();
	if (this->mpTextureView)
	{
		this->mpTextureView->releaseToPool();
		this->mpTextureView.reset();
	}
}

void EditorTexture::renderMenuBarItems()
{
	AssetEditor::renderMenuBarItems();
	if (ImGui::MenuItem("Import", "", false, this->mModalImport.expired()))
	{
		this->mModalImport = Editor::EDITOR->openNewGui<gui::modal::PathModal>("Import Image");
		this->mModalImport.lock()->setCallback(std::bind(&EditorTexture::onImportConfirmed, this, std::placeholders::_1));
	}
}

void EditorTexture::renderContent()
{
	AssetEditor::renderContent();

	auto asset = this->get<asset::Texture>();
	if (properties::renderProperty("Preview Scale", this->mViewScale, 1.0f))
	{
		this->mViewSize = this->mTextureSize;
	}
	if (properties::renderProperty("Preview Size", this->mViewSize, asset->getSourceSize()))
	{
		this->mViewScale = 1.0f;
	}

	if (this->mpTextureView && this->mpTextureView->isValid())
	{
		auto size = ImVec2(
			this->mViewSize.x() * this->mViewScale,
			this->mViewSize.y() * this->mViewScale
		);
		ImGui::Image(this->mpTextureView->id(), size);
	}
}

void EditorTexture::onImportConfirmed(std::filesystem::path path)
{
	auto asset = this->get<asset::Texture>();
	asset->setSourcePath(std::filesystem::relative(path, asset->getPath().parent_path()));
	this->saveAsset();
	this->loadPreview();
}

void EditorTexture::loadPreview()
{
	if (!this->mpTextureView) return;
	this->mpTextureView->invalidate();

	auto asset = this->get<asset::Texture>();
	this->mViewScale = 1.0f;

	auto srcPath = asset->getAbsoluteSourcePath();
	if (std::filesystem::exists(srcPath))
	{
		auto pixelData = std::vector<ui8>();
		build::BuildTexture::loadImage(srcPath, this->mTextureSize, pixelData);
		this->mViewSize = this->mTextureSize;
		this->mpTextureView->image()
			.setFormat(vk::Format::eR8G8B8A8Srgb)
			.setSize(math::Vector3UInt(this->mTextureSize).z(1))
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
		this->mpTextureView->create(pixelData);
	}
}
