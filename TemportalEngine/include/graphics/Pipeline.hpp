#pragma once

#include "graphics/DeviceObject.hpp"

#include "graphics/AttributeBinding.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class DescriptorGroup;
class RenderPass;
class ShaderModule;

struct BlendMode
{
	struct BlendComponent
	{
		vk::BlendOp operation;
		vk::BlendFactor srcFactor;
		vk::BlendFactor dstFactor;
	};
	vk::ColorComponentFlags writeMask;
	BlendComponent colorOp;
	BlendComponent alphaOp;
};

class Pipeline : public DeviceObject
{
	friend class VulkanApi;
	friend class Command;

public:
	Pipeline() = default;

	Pipeline& setBindings(std::vector<AttributeBinding> bindings);
	Pipeline& addShader(std::shared_ptr<ShaderModule> shader);
	Pipeline& setViewArea(vk::Viewport const &viewport, vk::Rect2D const &scissor);
	Pipeline& setFrontFace(vk::FrontFace const face);
	Pipeline& setBlendMode(std::optional<BlendMode> mode);

	Pipeline& setRenderPass(RenderPass *pRenderPass);
	Pipeline& setDescriptors(std::vector<DescriptorGroup*> descriptors);

	vk::Viewport const& getViewport() const;

	bool isValid() const;
	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

	void destroyShaders();

private:
	std::unordered_map<vk::ShaderStageFlagBits, std::shared_ptr<ShaderModule>> mShaderPtrs;
	std::vector<AttributeBinding> mAttributeBindings;

	vk::Viewport mViewport;
	vk::Rect2D mScissor;
	vk::FrontFace mFrontFace;
	std::optional<BlendMode> mBlendMode;

	vk::RenderPass mRenderPass;
	std::vector<vk::DescriptorSetLayout> mDescriptorLayouts;

	vk::UniquePipelineLayout mLayout;
	vk::UniquePipelineCache mCache;
	vk::UniquePipeline mPipeline;

	// Creates shader stage info items from `mpShaders`. Not safe to use if mpShaders change or any element is deleted before it is used.
	std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages() const;

};

NS_END
