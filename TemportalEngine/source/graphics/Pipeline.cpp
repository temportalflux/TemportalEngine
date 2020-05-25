#include "graphics/Pipeline.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/ShaderModule.hpp"
#include "graphics/RenderPass.hpp"
#include "types/integer.h"

using namespace graphics;

Pipeline& Pipeline::addShader(ShaderModule *pShader)
{
	this->mShaderPtrs.push_back(pShader);
	return *this;
}

std::vector<vk::PipelineShaderStageCreateInfo> Pipeline::createShaderStages() const
{
	auto shaderCount = this->mShaderPtrs.size();
	auto shaderStages = std::vector<vk::PipelineShaderStageCreateInfo>(shaderCount);
	for (uSize i = 0; i < shaderCount; ++i)
	{
		shaderStages[i] = this->mShaderPtrs[i]->getPipelineInfo();
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

Pipeline& Pipeline::create(LogicalDevice const *pDevice, RenderPass const *pRenderPass)
{
	// TODO (START): These need to go in configurable objects
	auto infoInputVertex = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(0)
		.setPVertexBindingDescriptions(nullptr)
		.setVertexAttributeDescriptionCount(0)
		.setPVertexAttributeDescriptions(nullptr);

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
		.setFrontFace(vk::FrontFace::eClockwise)
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

	this->mLayout = pDevice->mDevice->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(0).setPushConstantRangeCount(0)
	);
	this->mCache = pDevice->mDevice->createPipelineCacheUnique(vk::PipelineCacheCreateInfo());

	for (auto& shaderPtr : this->mShaderPtrs)
	{
		shaderPtr->create(pDevice);
	}
	auto stages = this->createShaderStages();
	
	auto infoPipeline = vk::GraphicsPipelineCreateInfo()
		.setStageCount((ui32)stages.size()).setPStages(stages.data())
		.setRenderPass(pRenderPass->mRenderPass.get())
		.setSubpass(0)
		.setLayout(mLayout.get())
		// Configurables
		.setPVertexInputState(&infoInputVertex)
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

	for (auto& shaderPtr : this->mShaderPtrs)
	{
		shaderPtr->destroy();
	}

	return *this;
}

void Pipeline::destroy()
{
	this->mPipeline.reset();
	this->mCache.reset();
	this->mLayout.reset();
}
