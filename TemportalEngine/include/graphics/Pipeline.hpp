#pragma once

#include "TemportalEnginePCH.hpp"

#include <vector>
#include <unordered_map>

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class RenderPass;
class ShaderModule;

class Pipeline
{
	friend class VulkanApi;
	friend class Command;

public:
	Pipeline() = default;

	Pipeline& addShader(vk::ShaderStageFlagBits stage, ShaderModule *pShader);
	Pipeline& setViewArea(vk::Viewport const &viewport, vk::Rect2D const &scissor);

	ShaderModule* getShader(vk::ShaderStageFlagBits stage);

	bool isValid() const;
	Pipeline& create(LogicalDevice const *pDevice, RenderPass const *pRenderPass);
	void destroy();

private:
	std::unordered_map<vk::ShaderStageFlagBits, ShaderModule*> mShaderPtrs;
	vk::Viewport mViewport;
	vk::Rect2D mScissor;

	vk::UniquePipelineLayout mLayout;
	vk::UniquePipelineCache mCache;
	vk::UniquePipeline mPipeline;

	// Creates shader stage info items from `mpShaders`. Not safe to use if mpShaders change or any element is deleted before it is used.
	std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages() const;

};

NS_END
