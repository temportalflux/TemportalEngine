#include "graphics/FrameBuffer.hpp"

#include "graphics/ImageView.hpp"
#include "graphics/LogicalDevice.hpp"
#include "graphics/RenderPass.hpp"

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
	this->mRenderPass = *reinterpret_cast<vk::RenderPass*>(pRenderPass->get());
	this->mResolution = pRenderPass->getScissorResolution();
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
		.setRenderPass(this->mRenderPass)
		.setAttachmentCount((ui32)this->mAttachments.size())
		.setPAttachments(this->mAttachments.data())
		.setWidth(this->mResolution.x())
		.setHeight(this->mResolution.y())
		.setLayers(1);
	mInternal = pDevice->mDevice->createFramebufferUnique(info);
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
