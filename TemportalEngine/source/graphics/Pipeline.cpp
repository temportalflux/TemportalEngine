#include "graphics/Pipeline.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/ShaderModule.hpp"
#include "graphics/RenderPass.hpp"
#include "types/integer.h"

using namespace graphics;

// Explicity deconstructor to make shaded_ptr notes
Pipeline::~Pipeline()
{
	// These will likely be the only references to the shader objects left, so they will automatically be deallocated
	this->mShaderPtrs.clear();
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

Pipeline& Pipeline::create(LogicalDevice const *pDevice, RenderPass const *pRenderPass)
{
	for (auto[stage, shader] : this->mShaderPtrs)
	{
		shader->create(pDevice);
	}

	auto vertexShader = this->getShader(vk::ShaderStageFlagBits::eVertex);
	assert(vertexShader != nullptr);
	vk::VertexInputBindingDescription binding = vertexShader->createBindings();
	std::vector<vk::VertexInputAttributeDescription> attributes = vertexShader->createAttributes();

	auto infoInputVertex = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(1)
		.setPVertexBindingDescriptions(&binding)
		.setVertexAttributeDescriptionCount((ui32)attributes.size())
		.setPVertexAttributeDescriptions(attributes.data());

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
