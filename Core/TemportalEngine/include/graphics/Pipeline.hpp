#pragma once

#include "graphics/DeviceObject.hpp"

#include "graphics/AttributeBinding.hpp"
#include "graphics/BlendMode.hpp"
#include "graphics/Area.hpp"
#include "graphics/Descriptor.hpp"
#include "graphics/types.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class DescriptorGroup;
class RenderPass;
class ShaderModule;

class Pipeline : public DeviceObject
{
	friend class VulkanApi;
	friend class Command;

public:
	Pipeline();

	Pipeline& setBindings(std::vector<AttributeBinding> bindings);
	Pipeline& addShader(std::shared_ptr<ShaderModule> shader);
	Pipeline& addViewArea(graphics::Viewport const &viewport, graphics::Area const &scissor);
	Pipeline& setFrontFace(graphics::FrontFace const face);
	Pipeline& setBlendMode(std::optional<BlendMode> mode);
	Pipeline& setTopology(graphics::PrimitiveTopology const topology);
	Pipeline& setLineWidth(f32 const& width);
	Pipeline& setDepthEnabled(bool test, bool write);

	Pipeline& setRenderPass(std::weak_ptr<RenderPass> pRenderPass);
	Pipeline& setDescriptors(std::vector<DescriptorGroup> *descriptors);
	Pipeline& setDescriptorLayouts(std::vector<graphics::DescriptorLayout const*> const& layouts);
	Pipeline& setDescriptorLayout(graphics::DescriptorLayout const& layout, uSize const& setCount);

	Pipeline& setResolution(math::Vector2UInt resoltion);

	bool isValid() const;
	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

	void destroyShaders();

private:
	std::unordered_map<vk::ShaderStageFlagBits, std::shared_ptr<ShaderModule>> mShaderPtrs;
	std::vector<AttributeBinding> mAttributeBindings;

	std::optional<f32> mEnforcedAspectRatio;
	std::vector<graphics::Viewport> mViewports;
	std::vector<graphics::Area> mScissors;
	graphics::FrontFace mFrontFace;
	std::optional<BlendMode> mBlendMode;
	graphics::PrimitiveTopology mTopology;
	f32 mLineWidth;

	bool mbDepthTest;
	bool mbDepthWrite;

	std::weak_ptr<graphics::RenderPass> mpRenderPass;
	math::Vector2 mResolutionFloat;
	std::vector<vk::DescriptorSetLayout> mDescriptorLayouts;

	vk::UniquePipelineLayout mLayout;
	vk::UniquePipelineCache mCache;
	vk::UniquePipeline mPipeline;

	// Creates shader stage info items from `mpShaders`. Not safe to use if mpShaders change or any element is deleted before it is used.
	std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages() const;

};

NS_END
