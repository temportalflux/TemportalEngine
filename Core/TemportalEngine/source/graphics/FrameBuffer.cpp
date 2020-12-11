#include "graphics/FrameBuffer.hpp"

#include "graphics/ImageView.hpp"
#include "graphics/GraphicsDevice.hpp"
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
	return *this;
}

FrameBuffer& FrameBuffer::setResolution(math::Vector2UInt const &resolution)
{
	this->mResolution = resolution;
	return *this;
}

FrameBuffer& FrameBuffer::addAttachment(ImageView *pView)
{
	this->mAttachments.push_back(extract<vk::ImageView>(pView));
	return *this;
}

FrameBuffer& FrameBuffer::create(std::shared_ptr<GraphicsDevice> device)
{
	this->mInternal = device->createFrameBuffer(
		vk::FramebufferCreateInfo()
		.setRenderPass(this->mRenderPass)
		.setAttachmentCount((ui32)this->mAttachments.size())
		.setPAttachments(this->mAttachments.data())
		.setWidth(this->mResolution.x())
		.setHeight(this->mResolution.y())
		.setLayers(1)
	);
	return *this;
}

void* FrameBuffer::get()
{
	return &this->mInternal.get();
}

void FrameBuffer::destroy()
{
	this->mInternal.reset();
	this->mRenderPass = vk::RenderPass();
	this->mAttachments.clear();
	this->mResolution = {};
}
