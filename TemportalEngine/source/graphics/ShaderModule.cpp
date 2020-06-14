#include "graphics/ShaderModule.hpp"

#include "graphics/LogicalDevice.hpp"
#include "types/integer.h"

#include <fstream>

using namespace graphics;

ShaderModule::ShaderModule()
	: mMainOpName("main")
{
	// TODO: This should not be controlled by the shader, but rather, by the input attributes. Instancing will need to make another one of these.
	this->mBinding = vk::VertexInputBindingDescription().setBinding(0).setInputRate(vk::VertexInputRate::eVertex);
}

ShaderModule::ShaderModule(ShaderModule &&other)
{
	*this = std::move(other);
}

ShaderModule::~ShaderModule()
{
	this->destroy();
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other)
{
	this->mStage = other.mStage;
	this->mBinary = std::move(other.mBinary);
	this->mMetadata = std::move(other.mMetadata);
	this->mAttributeByteCount = std::move(other.mAttributeByteCount);
	this->mMainOpName = other.mMainOpName;
	this->mInternal.swap(other.mInternal);
	this->mAttributes = std::unordered_map<std::string, vk::VertexInputAttributeDescription>(other.mAttributes.begin(), other.mAttributes.end());
	other.destroy();
	return *this;
}

ShaderModule& ShaderModule::setStage(vk::ShaderStageFlagBits stage)
{
	this->mStage = stage;
	return *this;
}

ShaderModule& ShaderModule::setBinary(std::vector<ui32> binary)
{
	this->mBinary = binary;
	return *this;
}

ShaderModule& ShaderModule::setMetadata(graphics::ShaderMetadata metadata)
{
	this->mMetadata = metadata;
#ifndef NDEBUG
	uSize totalByteCount = 0;
#endif
	for (auto& attribute : metadata.inputAttributes)
	{
		this->mAttributes.insert(std::make_pair(
			attribute.propertyName,
			vk::VertexInputAttributeDescription()
			.setBinding(0) // TODO: Should be configurable
			.setLocation(attribute.slot)
			.setFormat((vk::Format)attribute.format)
		));
#ifndef NDEBUG
		this->mAttributeByteCount.insert(std::make_pair(attribute.propertyName, attribute.byteCount));
		totalByteCount += attribute.byteCount;
#endif
	}
#ifndef NDEBUG
	this->mBinding.setStride((ui32)totalByteCount);
#endif
	return *this;
}

ShaderModule& ShaderModule::setVertexDescription(VertexDescription desc)
{
	// ByteCount/Size of incoming description must be the same as the total byte count & stride set forth by the metadata/binary compilation
	assert(desc.size == this->mBinding.stride);
	this->mBinding.setStride(desc.size);
	for (const auto& [propertyName, propDesc] : desc.attributes)
	{
		// Every provided description must already exist in the metadata from compilation
		assert(this->mAttributes.find(propertyName) != this->mAttributes.end());
		// Each property must match in size/byte-count to that in the metadata
		assert(this->mAttributeByteCount.find(propertyName) != this->mAttributeByteCount.end());
		assert(propDesc.size == this->mAttributeByteCount.find(propertyName)->second);
		// Actually save the offset in the structure for the desired attribute
		this->mAttributes[propertyName].setOffset((ui32)propDesc.offset);
	}
	return *this;
}

bool ShaderModule::isLoaded() const
{
	// Checks underlying structure for VK_NULL_HANDLE
	return (bool)this->mInternal;
}

void ShaderModule::create(LogicalDevice const *pDevice)
{
	assert(!isLoaded()); // assumes the shader is not loaded
	assert(pDevice != nullptr && pDevice->isValid());
	
	auto info = vk::ShaderModuleCreateInfo()
		.setCodeSize(sizeof(ui32) * this->mBinary.size())
		.setPCode(this->mBinary.data());
	this->mInternal = pDevice->mDevice->createShaderModuleUnique(info);
}

void ShaderModule::destroy()
{
	if (this->isLoaded())
	{
		this->mInternal.reset();
	}
}

vk::PipelineShaderStageCreateInfo ShaderModule::getPipelineInfo() const
{
	assert(isLoaded());
	return vk::PipelineShaderStageCreateInfo()
		.setStage(mStage).setPName(mMainOpName.c_str())
		.setModule(this->mInternal.get()).setPSpecializationInfo(nullptr);
}

// TODO: rename to getBindings
vk::VertexInputBindingDescription ShaderModule::createBindings() const
{
	return this->mBinding;
}

// TODO: rename to getAttributes
std::vector<vk::VertexInputAttributeDescription> ShaderModule::createAttributes() const
{
	std::vector<vk::VertexInputAttributeDescription> attributes(this->mAttributes.size());
	ui32 i = 0;
	for (auto[propertyName, desc] : this->mAttributes)
	{
		attributes[i++] = desc;
	}
	return attributes;
}
