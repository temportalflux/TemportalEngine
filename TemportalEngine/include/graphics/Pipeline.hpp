#pragma once

#include "TemportalEnginePCH.hpp"

#include <vector>

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class RenderPass;
class ShaderModule;

class Pipeline
{

public:
	Pipeline() = default;

	void addShader(ShaderModule const *pShader);
	Pipeline& setViewArea(vk::Viewport const &viewport, vk::Rect2D const &scissor);

	bool isValid() const;
	void create(LogicalDevice const *pDevice, RenderPass const *pRenderPass);
	void destroy();

private:
	std::vector<ShaderModule const *> mShaderPtrs;
	vk::Viewport mViewport;
	vk::Rect2D mScissor;

	vk::UniquePipelineLayout mLayout;
	vk::UniquePipelineCache mCache;
	vk::UniquePipeline mPipeline;

	// Creates shader stage info items from `mpShaders`. Not safe to use if mpShaders change or any element is deleted before it is used.
	std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages() const;

};

NS_END
