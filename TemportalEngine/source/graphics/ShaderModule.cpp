#include "graphics/ShaderModule.hpp"

#include "graphics/LogicalDevice.hpp"
#include "types/integer.h"

#include <fstream>

using namespace graphics;

ShaderModule::ShaderModule()
	: mMainOpName("main")
{
	// TODO: Make this dynamic based on shader compilation
	this->mBinding = vk::VertexInputBindingDescription().setBinding(0).setInputRate(vk::VertexInputRate::eVertex);
	// NOTE: These don't have the offsets. The user will need to provide a struct which maps location to structure-property-offset via `offsetof(Structure, Property)`
	this->mAttributes = {
		{ "position", vk::VertexInputAttributeDescription().setBinding(0).setLocation(0).setFormat(vk::Format::eR32G32Sfloat) },
		{ "color", vk::VertexInputAttributeDescription().setBinding(0).setLocation(1).setFormat(vk::Format::eR32G32B32Sfloat) }
	};
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
	this->mMainOpName = other.mMainOpName;
	this->mFileName = other.mFileName;
	this->mStage = other.mStage;
	this->mShader.swap(other.mShader);
	this->mAttributes = std::unordered_map<std::string, vk::VertexInputAttributeDescription>(other.mAttributes.begin(), other.mAttributes.end());
	other.destroy();
	return *this;
}

ShaderModule& ShaderModule::setStage(vk::ShaderStageFlagBits stage)
{
	this->mStage = stage;
	return *this;
}

ShaderModule& ShaderModule::setVertexDescription(VertexDescription vertexData)
{
	this->mBinding.setStride(vertexData.size);
	for (auto [propertyName, byteOffset] : vertexData.locationToDataOffset)
	{
		assert(this->mAttributes.find(propertyName) != this->mAttributes.end());
		this->mAttributes[propertyName].setOffset(byteOffset);
	}
	return *this;
}

ShaderModule& ShaderModule::setSource(std::string fileName)
{
	this->mFileName = fileName;
	return *this;
}

bool ShaderModule::isLoaded() const
{
	// Checks underlying structure for VK_NULL_HANDLE
	return (bool)this->mShader;
}

std::optional<std::vector<char>> ShaderModule::readBinary() const
{
	std::ifstream file(this->mFileName, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		return std::nullopt;
	}

	uSize fileSize = (uSize)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

void ShaderModule::create(LogicalDevice const *pDevice)
{
	assert(!isLoaded()); // assumes the shader is not loaded
	assert(pDevice != nullptr && pDevice->isValid());

	auto binary = this->readBinary();
	if (!binary.has_value())
	{
		//LogRenderer(logging::ECategory::LOGERROR, "Failed to read compiled SPIR-V shader: %s", filePath.c_str());
		return;
	}

	auto info = vk::ShaderModuleCreateInfo()
		.setCodeSize(binary.value().size())
		.setPCode(reinterpret_cast<ui32 const *>(binary.value().data()));
	mShader = pDevice->mDevice->createShaderModuleUnique(info);
}

void ShaderModule::destroy()
{
	if (this->isLoaded())
	{
		this->mShader.reset();
	}
}

vk::PipelineShaderStageCreateInfo ShaderModule::getPipelineInfo() const
{
	assert(isLoaded());
	return vk::PipelineShaderStageCreateInfo()
		.setStage(mStage).setPName(mMainOpName.c_str())
		.setModule(mShader.get()).setPSpecializationInfo(nullptr);
}

vk::VertexInputBindingDescription ShaderModule::createBindings() const
{
	return this->mBinding;
}

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
