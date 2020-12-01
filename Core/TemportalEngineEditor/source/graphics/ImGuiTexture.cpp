#include "graphics/ImGuiTexture.hpp"

#include "graphics/ImGuiRenderer.hpp"
#include "graphics/VulkanApi.hpp"
#include "graphics/types.hpp"

#include <backends/imgui_impl_vulkan.h>

using namespace graphics;

ImGuiTexture::ImGuiTexture(std::shared_ptr<ImGuiRenderer> renderer, ImTextureID textureId)
	: mpRenderer(renderer)
	, mTextureId(textureId)
{
	this->mImage.setDevice(renderer->getDevice());
	this->mView.setDevice(renderer->getDevice());
	this->mSampler.setDevice(renderer->getDevice());

	this->mSampler
		.setFilter(graphics::FilterMode::Enum::Nearest, graphics::FilterMode::Enum::Nearest)
		.setAddressMode({
			graphics::SamplerAddressMode::Enum::ClampToEdge,
			graphics::SamplerAddressMode::Enum::ClampToEdge,
			graphics::SamplerAddressMode::Enum::ClampToEdge
		})
		.setAnistropy(std::nullopt)
		.setBorderColor(graphics::BorderColor::Enum::BlackOpaqueInt)
		.setNormalizeCoordinates(true)
		.setCompare(graphics::CompareOp::Enum::Always)
		.setMipLOD(graphics::SamplerLODMode::Enum::Linear, 0, { 0, 0 });
	this->mSampler.create();
}

ImGuiTexture::~ImGuiTexture()
{
	this->mImage.destroy();
	this->mView.destroy();
	this->mSampler.destroy();
}

ImTextureID const& ImGuiTexture::id() const { return this->mTextureId; }
graphics::Image& ImGuiTexture::image() { return this->mImage; }
graphics::ImageView& ImGuiTexture::view() { return this->mView; }
graphics::ImageSampler& ImGuiTexture::sampler() { return this->mSampler; }

void ImGuiTexture::create(std::vector<ui8> const& pixels)
{
	assert(this->mTextureId != nullptr);
	this->mImage.create();

	auto& cmdPool = this->mpRenderer.lock()->getTransientPool();
	this->mImage.transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, &cmdPool);
	this->mImage.writeImage((void*)pixels.data(), pixels.size() * sizeof(ui8), &cmdPool);
	this->mImage.transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, &cmdPool);

	this->mView.setImage(&this->mImage, vk::ImageAspectFlagBits::eColor);
	this->mView.create();

	ImGui_ImplVulkan_UpdateTexture(
		this->mTextureId,
		extract<VkImageView>(&this->mView),
		extract<VkSampler>(&this->mSampler),
		(VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal
	);

	this->bHasCreated = true;
}

bool ImGuiTexture::isValid() const
{
	return this->bHasCreated;
}

void ImGuiTexture::invalidate()
{
	this->mView.invalidate();
	this->mView.resetConfiguration();
	this->mImage.invalidate();
	this->mImage.resetConfiguration();
	this->bHasCreated = false;
}

void ImGuiTexture::releaseToPool()
{
	this->invalidate();
	this->mpRenderer.lock()->releaseTextureToPool(this->shared_from_this());
}
