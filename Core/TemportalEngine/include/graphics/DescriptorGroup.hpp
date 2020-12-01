#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/Buffer.hpp"
#include "graphics/types.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class Buffer;
class DescriptorPool;
class GraphicsDevice;
class ImageSampler;
class ImageView;


class DescriptorGroup
{
	friend class GraphicsDevice;

public:
	DescriptorGroup();

	// Sets the total number of bindings
	DescriptorGroup& setBindingCount(uSize count);

	// Sets the number of expected sets that will be made for this layout
	// often corresponds to the number of frames
	DescriptorGroup& setAmount(ui32 setCount);
	
	// Sets the number of expected set duplicates
	// each is accessible by passing a `archetype index` to the `attachToBinding` function
	DescriptorGroup& setArchetypeAmount(uSize amt);

	/**
	 * Sets up a binding descriptor which can later be written to.
	 * `idx` specifies both the index in the list of bindings (max size dictated by `setBindingCount`)
	 *		AND the value of the binding in the shader it will be mapped to `layout(binding = #) uniform...`
	 */
	DescriptorGroup& addBinding(
		std::string const &id, uIndex const idx,
		graphics::DescriptorType const type,
		graphics::ShaderStage const shaderStage,
		ui32 count = 1 // note: useful for arrays of uniforms as a descriptor
	);

	DescriptorGroup& attachToBinding(
		std::string const &id,
		graphics::Buffer &buffer,
		uIndex idxArchetype = 0
	);

	// Attaches a buffer to each set created based on the `count` passed to `create`
	DescriptorGroup& attachToBinding(
		std::string const &id,
		std::vector<graphics::Buffer*> &buffers,
		uIndex idxArchetype = 0
	);

	DescriptorGroup& attachToBinding(
		std::string const &id,
		vk::ImageLayout const layout, ImageView *view, ImageSampler *sampler,
		uIndex idxArchetype = 0
	);

	DescriptorGroup& attachToBinding(
		std::string const &id, uIndex idxArchetype, uIndex idxSet,
		vk::ImageLayout const layout, ImageView *view, ImageSampler *sampler
	);

	DescriptorGroup& create(std::shared_ptr<GraphicsDevice> device, DescriptorPool *pool);
	DescriptorGroup& writeAttachments(std::shared_ptr<GraphicsDevice> device);
	void invalidate();

	vk::DescriptorSetLayout layout() const;
	vk::DescriptorSet const& getDescriptorSet(uIndex idxSet, uIndex idxArchetype=0) const;

private:
	// The actual descriptor bindings for how the descriptors attach to parts of the pipeline
	std::vector<vk::DescriptorSetLayoutBinding> mBindings;
	std::unordered_map<std::string, uIndex> mBindingIdxById;

	ui32 mSetCount;

	// The layout created from the bindings list and used to create the sets
	vk::UniqueDescriptorSetLayout mInternalLayout;

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
	struct SetArchetype
	{
		std::vector<WriteInstructions> mWriteInstructions;
		// The actual descriptor set(s) created from layout that can be bound for rendering.
		std::vector<vk::DescriptorSet> mInternalSets;
	};
	
	std::vector<SetArchetype> mArchetypes;

};

NS_END
