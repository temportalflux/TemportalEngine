#include "graphics/FrameBuffer.hpp"

#include "graphics/ImageView.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

FrameBuffer::FrameBuffer(FrameBuffer &&other)
{
	*this = std::move(other);
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other)
{
	mRenderPass = other.mRenderPass;
	mInternal.swap(other.mInternal);
	other.destroy();
	return *this;
}

FrameBuffer::~FrameBuffer()
{
	this->destroy();
}

FrameBuffer& FrameBuffer::setRenderPass(RenderPass *pRenderPass)
{
	this->mRenderPass = extract<vk::RenderPass>(pRenderPass);
	this->mResolution = pRenderPass->getScissorResolution();
	return *this;
}

FrameBuffer& FrameBuffer::addAttachment(ImageView *pView)
{
	this->mAttachments.push_back(extract<vk::ImageView>(pView));
	return *this;
}

FrameBuffer& FrameBuffer::create(LogicalDevice *pDevice)
{
	auto info = vk::FramebufferCreateInfo()
		.setRenderPass(this->mRenderPass)
		.setAttachmentCount((ui32)this->mAttachments.size())
		.setPAttachments(this->mAttachments.data())
		.setWidth(this->mResolution.x())
		.setHeight(this->mResolution.y())
		.setLayers(1);
	mInternal = extract<vk::Device>(pDevice).createFramebufferUnique(info);
	return *this;
}

void* FrameBuffer::get()
{
	return &this->mInternal.get();
}

void FrameBuffer::destroy()
{
	this->mInternal.reset();
}
