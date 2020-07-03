#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/AttributeBinding.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class DescriptorGroup;
class LogicalDevice;
class RenderPass;
class ShaderModule;

class Pipeline
{
	friend class VulkanApi;
	friend class Command;

public:
	Pipeline() = default;

	Pipeline& setBindings(std::vector<AttributeBinding> bindings);
	Pipeline& addShader(std::shared_ptr<ShaderModule> shader);
	Pipeline& setViewArea(vk::Viewport const &viewport, vk::Rect2D const &scissor);
	Pipeline& setFrontFace(vk::FrontFace const face);

	std::shared_ptr<ShaderModule> getShader(vk::ShaderStageFlagBits stage);
	vk::Viewport const& getViewport() const;

	bool isValid() const;
	Pipeline& create(LogicalDevice *pDevice, RenderPass *pRenderPass, std::vector<DescriptorGroup*> descriptors);
	void destroy();
	void clearShaders();

private:
	std::unordered_map<vk::ShaderStageFlagBits, std::shared_ptr<ShaderModule>> mShaderPtrs;
	vk::Viewport mViewport;
	vk::Rect2D mScissor;
	vk::FrontFace mFrontFace;

	vk::UniquePipelineLayout mLayout;
	vk::UniquePipelineCache mCache;
	vk::UniquePipeline mPipeline;

	std::vector<AttributeBinding> mAttributeBindings;

	// Creates shader stage info items from `mpShaders`. Not safe to use if mpShaders change or any element is deleted before it is used.
	std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages() const;

};

NS_END
