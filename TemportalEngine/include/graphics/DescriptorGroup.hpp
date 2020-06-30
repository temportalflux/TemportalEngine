#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/Buffer.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class Buffer;
class DescriptorPool;
class ImageSampler;
class ImageView;
class LogicalDevice;

class DescriptorGroup
{

public:
	DescriptorGroup() = default;

	// Sets the total number of bindings
	DescriptorGroup& setBindingCount(uSize count);

	// Sets the number of expected sets that will be made for this layout
	// often corresponds to the number of frames
	DescriptorGroup& setAmount(ui32 setCount);

	/**
	 * Sets up a binding descriptor which can later be written to.
	 * `idx` specifies both the index in the list of bindings (max size dictated by `setBindingCount`)
	 *		AND the value of the binding in the shader it will be mapped to `layout(binding = #) uniform...`
	 */
	DescriptorGroup& addBinding(
		ui32 const idx,
		vk::DescriptorType const type,
		vk::ShaderStageFlags const shaderStage,
		ui32 count = 1 // note: useful for arrays of uniforms as a descriptor
	);

	DescriptorGroup& attachToBinding(
		ui32 const idx,
		graphics::Buffer &buffer, ui32 const offset
	);

	// Attaches a buffer to each set created based on the `count` passed to `create`
	DescriptorGroup& attachToBinding(
		ui32 const idx,
		std::vector<graphics::Buffer> &buffers, ui32 const offset
	);

	DescriptorGroup& attachToBinding(
		ui32 const idx,
		vk::ImageLayout const layout, ImageView *view, ImageSampler *sampler
	);

	DescriptorGroup& create(LogicalDevice *device, DescriptorPool *pool);
	DescriptorGroup& writeAttachments(LogicalDevice *device);
	void invalidate();

	vk::DescriptorSetLayout layout() const;
	const vk::DescriptorSet& operator[](uIndex idxSet) const;

private:
	// The actual descriptor bindings for how the descriptors attach to parts of the pipeline
	std::vector<vk::DescriptorSetLayoutBinding> mBindings;

	ui32 mSetCount;

	// The configurations for writing to the descriptor set(s) once they are created
	struct WriteInstructions
	{
		std::vector<vk::WriteDescriptorSet> writes;
		std::vector<vk::DescriptorBufferInfo> infoBuffers;
		std::vector<vk::DescriptorImageInfo> infoImages;

		vk::DescriptorBufferInfo& pushBufferInfo()
		{
			uIndex idx = infoBuffers.size();
			infoBuffers.push_back(vk::DescriptorBufferInfo());
			return infoBuffers[idx];
		}

		vk::DescriptorImageInfo& pushImageInfo()
		{
			uIndex idx = infoImages.size();
			infoImages.push_back(vk::DescriptorImageInfo());
			return infoImages[idx];
		}
	};
	std::vector<WriteInstructions> mWriteInstructions;
	
	// The layout created from the bindings list and used to create the sets
	vk::UniqueDescriptorSetLayout mInternalLayout;
	
	// The actual descriptor set(s) created from layout that can be bound for rendering.
	// The amount depends on the `count` passed into `create`.
	std::vector<vk::DescriptorSet> mInternalSets;


};

NS_END
