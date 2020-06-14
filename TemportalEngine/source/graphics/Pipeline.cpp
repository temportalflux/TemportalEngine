#include "graphics/Pipeline.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/ShaderModule.hpp"
#include "graphics/RenderPass.hpp"
#include "types/integer.h"

using namespace graphics;

Pipeline& Pipeline::setBindings(std::vector<AttributeBinding> bindings)
{
	this->mAttributeBindings = bindings;
	return *this;
}

Pipeline& Pipeline::addShader(std::shared_ptr<ShaderModule> shader)
{
	this->mShaderPtrs.insert(std::make_pair(shader->mStage, shader));
	return *this;
}

std::shared_ptr<ShaderModule> Pipeline::getShader(vk::ShaderStageFlagBits stage)
{
	auto iter = this->mShaderPtrs.find(stage);
	return iter != this->mShaderPtrs.end() ? iter->second : nullptr;
}

std::vector<vk::PipelineShaderStageCreateInfo> Pipeline::createShaderStages() const
{
	auto shaderCount = this->mShaderPtrs.size();
	auto shaderStages = std::vector<vk::PipelineShaderStageCreateInfo>(shaderCount);
	uSize i = 0;
	for (auto [stage, shader] : this->mShaderPtrs)
	{
		shaderStages[i++] = shader->getPipelineInfo();
	}
	return shaderStages;
}

Pipeline& Pipeline::setViewArea(vk::Viewport const &viewport, vk::Rect2D const &scissor)
{
	this->mViewport = viewport;
	this->mScissor = scissor;
	return *this;
}

bool Pipeline::isValid() const
{
	return (bool)this->mPipeline;
}

Pipeline& Pipeline::create(LogicalDevice const *pDevice, RenderPass const *pRenderPass, std::vector<vk::DescriptorSetLayout> layouts)
{
	for (auto[stage, shader] : this->mShaderPtrs)
	{
		shader->create(pDevice);
	}

	auto bindingCount = (ui32)this->mAttributeBindings.size();
	auto bindingDescs = std::vector<vk::VertexInputBindingDescription>();
	auto attribDescs = std::vector<vk::VertexInputAttributeDescription>();
	for (ui32 i = 0; i < bindingCount; ++i)
	{
		bindingDescs.push_back(
			vk::VertexInputBindingDescription().setBinding(i)
			.setInputRate((vk::VertexInputRate)this->mAttributeBindings[i].mInputRate)
			.setStride(this->mAttributeBindings[i].mSize)
		);
		for (const auto& attribute : this->mAttributeBindings[i].mAttributes)
		{
			attribDescs.push_back(
				vk::VertexInputAttributeDescription().setBinding(i)
				.setLocation(attribute.slot)
				.setFormat((vk::Format)attribute.format)
				.setOffset(attribute.offset)
			);
		}
	}
	auto vertexBindingInfo = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount((ui32)bindingDescs.size())
		.setPVertexBindingDescriptions(bindingDescs.data())
		.setVertexAttributeDescriptionCount((ui32)attribDescs.size())
		.setPVertexAttributeDescriptions(attribDescs.data());

	// TODO (START): These need to go in configurable objects
	auto infoAssembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setPrimitiveRestartEnable(false);

	auto infoViewportState = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setPViewports(&mViewport)
		.setScissorCount(1)
		.setPScissors(&mScissor);

	auto infoRasterization = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1.0f)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f);

	auto infoMultisampling = vk::PipelineMultisampleStateCreateInfo()
		.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setPSampleMask(nullptr);

	auto infoColorBlendAttachment = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(
			vk::ColorComponentFlagBits::eR
			| vk::ColorComponentFlagBits::eG
			| vk::ColorComponentFlagBits::eB
			| vk::ColorComponentFlagBits::eA
		)
		.setBlendEnable(false);

	auto infoColorBlendState = vk::PipelineColorBlendStateCreateInfo()
		.setLogicOpEnable(false)
		.setLogicOp(vk::LogicOp::eCopy)
		.setAttachmentCount(1)
		.setPAttachments(&infoColorBlendAttachment)
		.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

	vk::DynamicState dynamicStates[] = { vk::DynamicState::eViewport, vk::DynamicState::eLineWidth };
	auto infoDynamicStates = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStateCount(2)
		.setPDynamicStates(dynamicStates);
	// TODO (END)

	this->mLayout = pDevice->mDevice->createPipelineLayoutUnique(
		vk::PipelineLayoutCreateInfo().setPushConstantRangeCount(0)
		.setSetLayoutCount((ui32)layouts.size()).setPSetLayouts(layouts.data())
	);
	this->mCache = pDevice->mDevice->createPipelineCacheUnique(vk::PipelineCacheCreateInfo());

	auto stages = this->createShaderStages();
	
	auto infoPipeline = vk::GraphicsPipelineCreateInfo()
		.setStageCount((ui32)stages.size()).setPStages(stages.data())
		.setRenderPass(pRenderPass->mRenderPass.get())
		.setSubpass(0)
		.setLayout(mLayout.get())
		// Configurables
		.setPVertexInputState(&vertexBindingInfo)
		.setPInputAssemblyState(&infoAssembly)
		.setPViewportState(&infoViewportState)
		.setPRasterizationState(&infoRasterization)
		.setPMultisampleState(&infoMultisampling)
		.setPColorBlendState(&infoColorBlendState)
		.setPDepthStencilState(nullptr)
		.setPDynamicState(nullptr)
		//.setPDynamicState(&infoDynamicStates)
		.setBasePipelineHandle({});

	this->mPipeline = pDevice->mDevice->createGraphicsPipelineUnique(this->mCache.get(), infoPipeline);

	for (auto[stage, shader] : this->mShaderPtrs)
	{
		shader->destroy();
	}

	return *this;
}

void Pipeline::destroy()
{
	this->mPipeline.reset();
	this->mCache.reset();
	this->mLayout.reset();
}

void Pipeline::clearShaders()
{
	// These will likely be the only references to the shader objects left, so they will automatically be deallocated
	this->mShaderPtrs.clear();
}
