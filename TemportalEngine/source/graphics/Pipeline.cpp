#include "graphics/Pipeline.hpp"

#include "graphics/DescriptorGroup.hpp"
#include "graphics/GraphicsDevice.hpp"
#include "graphics/ShaderModule.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/VulkanApi.hpp"

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

vk::Viewport const& Pipeline::getViewport() const
{
	return this->mViewport;
}

Pipeline& Pipeline::setFrontFace(vk::FrontFace const face)
{
	this->mFrontFace = face;
	return *this;
}

Pipeline& Pipeline::setBlendMode(std::optional<BlendMode> mode)
{
	this->mBlendMode = mode;
	return *this;
}

Pipeline& Pipeline::setRenderPass(RenderPass *pRenderPass)
{
	this->mRenderPass = graphics::extract<vk::RenderPass>(pRenderPass);
	return *this;
}

Pipeline& Pipeline::setDescriptors(std::vector<DescriptorGroup*> descriptors)
{
	this->mDescriptorLayouts.resize(descriptors.size());
	std::transform(
		descriptors.begin(), descriptors.end(), this->mDescriptorLayouts.begin(),
		[](DescriptorGroup* descriptor) { return descriptor->layout(); }
	);
	return *this;
}

bool Pipeline::isValid() const
{
	return (bool)this->mPipeline;
}

void Pipeline::create()
{
	for (auto[stage, shader] : this->mShaderPtrs)
	{
		shader->create(this->device());
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
		.setFrontFace(this->mFrontFace)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f);

	auto infoMultisampling = vk::PipelineMultisampleStateCreateInfo()
		.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setPSampleMask(nullptr);

	auto infoColorBlendAttachment = vk::PipelineColorBlendAttachmentState();

	if (this->mBlendMode)
	{
		infoColorBlendAttachment
			.setBlendEnable(true)
			.setColorWriteMask(this->mBlendMode->writeMask)
			.setColorBlendOp(this->mBlendMode->colorOp.operation)
			.setSrcColorBlendFactor(this->mBlendMode->colorOp.srcFactor)
			.setDstColorBlendFactor(this->mBlendMode->colorOp.dstFactor)
			.setAlphaBlendOp(this->mBlendMode->alphaOp.operation)
			.setSrcAlphaBlendFactor(this->mBlendMode->alphaOp.srcFactor)
			.setDstAlphaBlendFactor(this->mBlendMode->alphaOp.dstFactor);
	}
	else
	{
		infoColorBlendAttachment
			.setColorWriteMask(
				vk::ColorComponentFlagBits::eR
				| vk::ColorComponentFlagBits::eG
				| vk::ColorComponentFlagBits::eB
				| vk::ColorComponentFlagBits::eA
			)
			.setBlendEnable(false);
	}

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

	auto infoDepthStencil = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(true).setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthBoundsTestEnable(false).setMinDepthBounds(0.0f).setMaxDepthBounds(1.0f)
		.setStencilTestEnable(false).setFront(vk::StencilOpState()).setBack(vk::StencilOpState());
	// TODO (END)

	this->mLayout = this->device()->createPipelineLayout(
		vk::PipelineLayoutCreateInfo()
		.setPushConstantRangeCount(0)
		.setSetLayoutCount((ui32)this->mDescriptorLayouts.size())
		.setPSetLayouts(this->mDescriptorLayouts.data())
	);
	this->mCache = this->device()->createPipelineCache(vk::PipelineCacheCreateInfo());

	auto stages = this->createShaderStages();
	
	auto infoPipeline = vk::GraphicsPipelineCreateInfo()
		.setStageCount((ui32)stages.size()).setPStages(stages.data())
		.setRenderPass(this->mRenderPass)
		.setSubpass(0)
		.setLayout(mLayout.get())
		// Configurables
		.setPVertexInputState(&vertexBindingInfo)
		.setPInputAssemblyState(&infoAssembly)
		.setPViewportState(&infoViewportState)
		.setPRasterizationState(&infoRasterization)
		.setPMultisampleState(&infoMultisampling)
		.setPColorBlendState(&infoColorBlendState)
		.setPDepthStencilState(&infoDepthStencil)
		.setPDynamicState(nullptr)
		//.setPDynamicState(&infoDynamicStates)
		.setBasePipelineHandle({});

	this->mPipeline = this->device()->createPipeline(this->mCache, infoPipeline);

	for (auto[stage, shader] : this->mShaderPtrs)
	{
		shader->destroy();
	}
}

void* Pipeline::get()
{
	return &this->mPipeline.get();
}

void Pipeline::invalidate()
{
	this->mPipeline.reset();
	this->mCache.reset();
	this->mLayout.reset();
}

void Pipeline::resetConfiguration()
{
	this->mViewport = vk::Viewport();
	this->mScissor = vk::Rect2D();
	this->mFrontFace = vk::FrontFace();
	this->mBlendMode = std::nullopt;
	this->mRenderPass = vk::RenderPass();
	this->mDescriptorLayouts.clear();
}

void Pipeline::destroyShaders()
{
	// These will likely be the only references to the shader objects left, so they will automatically be deallocated
	this->mShaderPtrs.clear();
}
