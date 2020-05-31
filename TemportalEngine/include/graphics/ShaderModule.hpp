#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;

// TODO: This class is a great base template as a starting point for abstracting out information about binary assets and serialization
// readBinary is already implmented and could be abstracted out to a super class.
// Could even add converter methods to read a binary file and write as plain text or vice-versa.
// TODO: Can integrate https://github.com/google/shaderc for compiling shaders within the editor
class ShaderModule
{
	friend class Pipeline;
	friend class VulkanApi;

public:
	struct VertexDescription
	{
		// Total size of 1 vertex structure (all the attributes for 1 vertex)
		ui32 size;
		// map of attribute name to the byte-offset of the property in the user's structure
		std::unordered_map<std::string, ui32> locationToDataOffset;
	};

	ShaderModule();
	ShaderModule(ShaderModule &&other);
	~ShaderModule();
	ShaderModule& operator=(ShaderModule&& other);

	ShaderModule& setStage(vk::ShaderStageFlagBits stage);
	ShaderModule& setSource(std::string fileName);

	ShaderModule& setVertexDescription(VertexDescription vertexData);

	bool isLoaded() const;
	void create(LogicalDevice const *pDevice);
	void destroy();

private:
	std::string mFileName;
	std::string mMainOpName;
	vk::ShaderStageFlagBits mStage;
	vk::UniqueShaderModule mShader;

	vk::VertexInputBindingDescription mBinding;
	std::unordered_map<std::string, vk::VertexInputAttributeDescription> mAttributes;

	std::optional<std::vector<char>> readBinary() const;
	vk::PipelineShaderStageCreateInfo getPipelineInfo() const;
	vk::VertexInputBindingDescription createBindings() const;
	std::vector<vk::VertexInputAttributeDescription> createAttributes() const;

};

NS_END
