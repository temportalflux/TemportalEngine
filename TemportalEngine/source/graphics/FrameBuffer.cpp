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
	mBuffer.swap(other.mBuffer);
	other.destroy();
	return *this;
}

FrameBuffer& FrameBuffer::setRenderPass(RenderPass const *pRenderPass)
{
	mpRenderPass = pRenderPass;
	return *this;
}

FrameBuffer& FrameBuffer::setView(vk::ImageView &view)
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
	mBuffer = pDevice->mDevice->createFramebufferUnique(info);
	return *this;
}

void FrameBuffer::destroy()
{
	this->mBuffer.reset();
	this->mpRenderPass = nullptr;
	this->mView = vk::ImageView();
}

vk::Framebuffer FrameBuffer::getBuffer() const
{
	return mBuffer.get();
}
