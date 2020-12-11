#include "graphics/Descriptor.hpp"

#include "graphics/Buffer.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/DescriptorPool.hpp"
#include "graphics/VulkanApi.hpp"

#include <vulkan/vulkan.hpp>

using namespace graphics;

DescriptorLayout::DescriptorLayout() : mInternal(nullptr)
{
}

DescriptorLayout::DescriptorLayout(DescriptorLayout &&other)
{
	*this = std::move(other);
}

DescriptorLayout& DescriptorLayout::operator=(DescriptorLayout &&other)
{
	this->mpDevice = other.mpDevice;
	other.mpDevice.reset();
	this->mBindings = std::move(other.mBindings);
	this->mInternal = other.mInternal;
	other.mInternal = nullptr;
	return *this;
}

DescriptorLayout::~DescriptorLayout()
{
	invalidate();
}

DescriptorLayout& DescriptorLayout::setDevice(std::weak_ptr<GraphicsDevice> device)
{
	this->mpDevice = device;
	return *this;
}

DescriptorLayout& DescriptorLayout::setBindingCount(uSize count)
{
	this->mBindings.clear();
	this->mBindings.resize(count);
	return *this;
}

DescriptorLayout& DescriptorLayout::setBinding(
	uIndex const idx, std::string const &id,
	graphics::DescriptorType const type,
	graphics::ShaderStage const shaderStage,
	ui32 const& count
)
{
	assert(idx < this->mBindings.size());
	auto& binding = this->mBindings[idx];
	binding.id = id;
	binding.type = type;
	binding.stage = shaderStage;
	binding.count = count;
	return *this;
}

DescriptorLayout& DescriptorLayout::create()
{
	auto bindingCount = (ui32)this->mBindings.size();
	auto bindings = std::vector<vk::DescriptorSetLayoutBinding>(bindingCount);
	for (uIndex idx = 0; idx < bindingCount; ++idx)
	{
		auto const& binding = this->mBindings[idx];
		bindings[idx]
			.setBinding(ui32(idx))
			.setDescriptorType(binding.type.as<vk::DescriptorType>())
			.setStageFlags(binding.stage.as<vk::ShaderStageFlagBits>())
			.setDescriptorCount(binding.count)
			.setPImmutableSamplers(nullptr);
	}
	this->mInternal = (void*)this->mpDevice.lock()->createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount(bindingCount)
		.setPBindings(bindings.data())
	);
	return *this;
}

void* DescriptorLayout::get() const
{
	return this->mInternal;
}

void DescriptorLayout::invalidate()
{
	if (this->mInternal != nullptr)
	{
		this->mpDevice.lock()->destroyDescriptorSetLayout(
			reinterpret_cast<VkDescriptorSetLayout>(this->mInternal)
		);
		this->mInternal = nullptr;
	}
}

DescriptorLayout& DescriptorLayout::createSet(DescriptorPool *pool, DescriptorSet &outSet)
{
	OPTICK_EVENT();
	auto vkSets = this->mpDevice.lock()->allocateDescriptorSets(
		pool, reinterpret_cast<VkDescriptorSetLayout>(this->mInternal), 1
	);
	outSet = DescriptorSet(this->mpDevice, this->mBindings, (void*)vkSets[0]);
	return *this;
}

DescriptorLayout& DescriptorLayout::createSets(DescriptorPool *pool, std::vector<DescriptorSet> &outSets)
{
	OPTICK_EVENT();
	auto vkSets = this->mpDevice.lock()->allocateDescriptorSets(
		pool, reinterpret_cast<VkDescriptorSetLayout>(this->mInternal), (ui32)outSets.size()
	);
	for (uIndex idxSet = 0; idxSet < outSets.size(); ++idxSet)
	{
		outSets[idxSet] = std::move(DescriptorSet(this->mpDevice, this->mBindings, (void*)vkSets[idxSet]));
	}
	return *this;
}

DescriptorSet::DescriptorSet() : mInternal(nullptr)
{
}

DescriptorSet::DescriptorSet(
	std::weak_ptr<GraphicsDevice> device,
	std::vector<DescriptorBinding> const& bindings,
	/*vk::DescriptorSet*/ void* internalSet
) : mpDevice(device), mBindings(bindings), mInternal(internalSet)
{
}

DescriptorSet::DescriptorSet(DescriptorSet &&other)
{
	*this = std::move(other);
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet &&other)
{
	this->mpDevice = other.mpDevice;
	other.mpDevice.reset();
	this->mBindings = std::move(other.mBindings);
	this->mWriteInstructions = std::move(other.mWriteInstructions);
	this->mInternal = other.mInternal;
	other.mInternal = nullptr;
	return *this;
}

DescriptorSet::~DescriptorSet()
{
}

std::optional<uIndex> DescriptorSet::findBinding(std::string const& id) const
{
	for (uIndex idx = 0; idx < this->mBindings.size(); ++idx)
	{
		if (this->mBindings[idx].id == id) return idx;
	}
	return std::nullopt;
}

DescriptorSet& DescriptorSet::attach(
	std::string const &id, graphics::Buffer *buffer
)
{
	auto foundIdx = this->findBinding(id);
	assert(foundIdx);
	auto instruction = WriteInstruction { *foundIdx };
	{
		auto& writeInfo = instruction.writeInfo.buffer;
		writeInfo.buffer = buffer;
		writeInfo.offset = 0;
		writeInfo.size = buffer->getSize();
	}
	this->mWriteInstructions.push_back(std::move(instruction));
	return *this;
}

DescriptorSet& DescriptorSet::attach(
	std::string const &id,
	ImageLayout const layout, ImageView *view, ImageSampler *sampler
)
{
	auto foundIdx = this->findBinding(id);
	assert(foundIdx);
	auto instruction = WriteInstruction{ *foundIdx };
	{
		auto& writeInfo = instruction.writeInfo.image;
		writeInfo.view = view;
		writeInfo.sampler = sampler;
		writeInfo.layout = layout;
	}
	this->mWriteInstructions.push_back(std::move(instruction));
	return *this;
}

DescriptorSet& DescriptorSet::writeAttachments()
{
	auto writeCount = this->mWriteInstructions.size();
	auto writes = std::vector<vk::WriteDescriptorSet>(writeCount);
	auto writeBufferInfo = std::vector<vk::DescriptorBufferInfo>();
	auto writeImageInfo = std::vector<vk::DescriptorImageInfo>();
	for (uIndex idxWrite = 0; idxWrite < writeCount; ++idxWrite)
	{
		auto const& instruction = this->mWriteInstructions[idxWrite];
		auto const& binding = this->mBindings[instruction.idxDescriptor];
		auto& write = writes[idxWrite]
			.setDstSet(reinterpret_cast<VkDescriptorSet>(this->mInternal))
			.setDstBinding((ui32)instruction.idxDescriptor)
			.setDescriptorType(binding.type.as<vk::DescriptorType>())
			.setDescriptorCount(binding.count)
			.setDstArrayElement(0);
		switch (binding.type.value())
		{
			case EDescriptorType::eUniformBuffer:
			{
				uIndex idx = writeBufferInfo.size();
				writeBufferInfo.push_back(
					vk::DescriptorBufferInfo()
					.setBuffer(graphics::extract<vk::Buffer>(instruction.writeInfo.buffer.buffer))
					.setOffset(instruction.writeInfo.buffer.offset)
					.setRange(instruction.writeInfo.buffer.size)
				);
				write.setPBufferInfo(&writeBufferInfo[idx]);
				break;
			}
			case EDescriptorType::eCombinedImageSampler:
			{
				uIndex idx = writeImageInfo.size();
				writeImageInfo.push_back(
					vk::DescriptorImageInfo()
					.setImageLayout(instruction.writeInfo.image.layout.as<vk::ImageLayout>())
					.setImageView(graphics::extract<vk::ImageView>(instruction.writeInfo.image.view))
					.setSampler(graphics::extract<vk::Sampler>(instruction.writeInfo.image.sampler))
				);
				write.setPImageInfo(&writeImageInfo[idx]);
				break;
			}
			default:
				// No other descriptor types are currently supported
				assert(false);
				break;
		}
	}
	
	auto copies = std::vector<vk::CopyDescriptorSet>();
	this->mpDevice.lock()->updateDescriptorSets(writes, copies);
	this->mWriteInstructions.clear();
	return *this;
}

void* DescriptorSet::get() const
{
	return this->mInternal;
}

DescriptorSetPool::DescriptorSetPool(DescriptorPool *descriptorPool)
	: mpDescriptorPool(descriptorPool)
{
}

DescriptorLayout& DescriptorSetPool::layout() { return this->mLayout; }


DynamicHandle<DescriptorSet> DescriptorSetPool::createHandle()
{
	uIndex idxSet;
	if (this->mUnusedSetIndices.size() > 0)
	{
		auto iter = this->mUnusedSetIndices.begin();
		idxSet = *iter;
		this->mUnusedSetIndices.erase(iter);
	}
	else
	{
		idxSet = this->mSets.size();
		
		DescriptorSet set = {};
		this->mLayout.createSet(this->mpDescriptorPool, set);
		this->mSets.push_back(std::move(set));
	}
	return DynamicHandle<DescriptorSet>(this->weak_from_this(), idxSet);
}

DescriptorSet* DescriptorSetPool::get(uIndex const& idx)
{
	return &this->mSets[idx];
}

void DescriptorSetPool::destroyHandle(uIndex const& idx)
{
	this->mUnusedSetIndices.insert(idx);
}
