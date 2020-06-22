#include "graphics/FrameBuffer.hpp"

#include "graphics/LogicalDevice.hpp"
#include "graphics/RenderPass.hpp"

using namespace graphics;

FrameBuffer::FrameBuffer(FrameBuffer &&other)
{
	*this = std::move(other);
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other)
{
	mpRenderPass = other.mpRenderPass;
	mpView = other.mpView;
	mInternal.swap(other.mInternal);
	other.destroy();
	return *this;
}

FrameBuffer::~FrameBuffer()
{
	this->destroy();
}

FrameBuffer& FrameBuffer::setRenderPass(RenderPass const *pRenderPass)
{
	mpRenderPass = pRenderPass;
	return *this;
}

FrameBuffer& FrameBuffer::addAttachment(ImageView *pView)
{
	this->mAttachments.push_back(*reinterpret_cast<vk::ImageView*>(pView->get()));
	return *this;
}

FrameBuffer& FrameBuffer::create(LogicalDevice const *pDevice)
{
	auto info = vk::FramebufferCreateInfo()
		.setRenderPass(mpRenderPass->mRenderPass.get())
		.setAttachmentCount((ui32)this->mAttachments.size())
		.setPAttachments(this->mAttachments.data())
		.setWidth(mpRenderPass->mResolution.width)
		.setHeight(mpRenderPass->mResolution.height)
		.setLayers(1);
	mInternal = pDevice->mDevice->createFramebufferUnique(info);
	return *this;
}

void FrameBuffer::destroy()
{
	this->mInternal.reset();
	this->mpRenderPass = nullptr;
	this->mpView = nullptr;
}

vk::Framebuffer FrameBuffer::getBuffer() const
{
	return mInternal.get();
}
