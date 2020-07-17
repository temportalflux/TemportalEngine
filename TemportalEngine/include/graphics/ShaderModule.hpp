#pragma once

#include "graphics/DeviceObject.hpp"

#include "graphics/ShaderMetadata.hpp"
#include "graphics/VertexDescription.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class ShaderModule : public DeviceObject
{
	friend class Pipeline;
	friend class VulkanApi;

public:

	ShaderModule();
	ShaderModule(ShaderModule &&other);
	~ShaderModule();
	ShaderModule& operator=(ShaderModule&& other);

	ShaderModule& setStage(vk::ShaderStageFlagBits stage);
	ShaderModule& setBinary(std::vector<ui32> binary);
	ShaderModule& setMetadata(graphics::ShaderMetadata metadata);

	/**
	 * Tells the shader module what the byte offset is for a given property from the data that will be sent for rendering.
	 * The shader must already have metadata initialized, which acts as the bridge from user defined structure to layout location.
	 */
	ShaderModule& setVertexDescription(VertexDescription desc);

	bool isValid() const;
	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

private:
	vk::ShaderStageFlagBits mStage;
	std::vector<ui32> mBinary;
	graphics::ShaderMetadata mMetadata;
	std::string mMainOpName;
#ifndef NDEBUG
	std::unordered_map<std::string, uSize> mAttributeByteCount;
#endif
	vk::VertexInputBindingDescription mBinding;
	std::unordered_map<std::string, vk::VertexInputAttributeDescription> mAttributes;

	vk::UniqueShaderModule mInternal;

	vk::PipelineShaderStageCreateInfo getPipelineInfo() const;

};

NS_END
