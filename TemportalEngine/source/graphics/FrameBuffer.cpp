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
	mView = other.mView;
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

FrameBuffer& FrameBuffer::setView(vk::ImageView const &view)
{
	mView = view;
	return *this;
}

FrameBuffer& FrameBuffer::create(LogicalDevice const *pDevice)
{
	auto info = vk::FramebufferCreateInfo()
		.setRenderPass(mpRenderPass->mRenderPass.get())
		.setAttachmentCount(1)
		.setPAttachments(&mView)
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
	this->mView = vk::ImageView();
}

vk::Framebuffer FrameBuffer::getBuffer() const
{
	return mInternal.get();
}
