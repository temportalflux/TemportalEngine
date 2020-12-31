#include "ui/ImageWidget.hpp"

#include "math/Matrix.hpp"
#include "graphics/Command.hpp"
#include "graphics/ImageSampler.hpp"

using namespace ui;

ImageResource::ImageResource()
{
	this->mImage
		.setFormat(vk::Format::eR8G8B8A8Srgb)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
}

ImageResource::ImageResource(ImageResource&& other) { *this = std::move(other); }

ImageResource& ImageResource::operator=(ImageResource &&other)
{
	this->mPixels = other.mPixels;
	this->mImageSize = other.mImageSize;
	this->mImage = std::move(other.mImage);
	this->mView = std::move(other.mView);
	this->mDescriptor = std::move(other.mDescriptor);
	return *this;
}

ImageResource::~ImageResource()
{
	this->release();
}

void ImageResource::release()
{
	this->mView.destroy();
	this->mImage.destroy();
}

ImageResource& ImageResource::setTexturePixels(std::vector<ui8> const& srgbPixels)
{
	this->mPixels = srgbPixels;
	return *this;
}

ImageResource& ImageResource::setTextureSize(math::Vector2UInt const& sizeInPixels)
{
	this->mImageSize = sizeInPixels;
	this->mImage.setSize(math::Vector3UInt(sizeInPixels).z(1));
	return *this;
}

ImageResource& ImageResource::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mImage.setDevice(device);
	this->mView.setDevice(device);
	return *this;
}

ImageResource& ImageResource::create(graphics::CommandPool* transientPool)
{
	this->mImage.create();

	this->mImage.transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, transientPool);
	this->mImage.writeImage((void*)this->mPixels.data(), this->mPixels.size() * sizeof(ui8), transientPool);
	this->mImage.transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, transientPool);
	this->mPixels.clear();

	this->mView.setImage(&this->mImage, vk::ImageAspectFlagBits::eColor).create();
	return *this;
}

ImageResource& ImageResource::createDescriptor(graphics::DescriptorLayout *layout, graphics::DescriptorPool *descriptorPool)
{
	layout->createSet(descriptorPool, this->mDescriptor);
	return *this;
}

ImageResource& ImageResource::attachWithSampler(graphics::ImageSampler *sampler)
{
	this->mDescriptor.attach("imgSampler", graphics::EImageLayout::eShaderReadOnlyOptimal, &this->mView, sampler);
	this->mDescriptor.writeAttachments();
	return *this;
}

Image::Image() : Widget(), mColor(1.0f), mSlicing({})
{
	this->mVertices.resize(/*maximum number of vertices given how padding and slicing works*/ 16);
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, graphics::MemoryUsage::eGPUOnly)
		.setSize(this->mVertices.size() * sizeof(Image::Vertex));
	this->mIndices.resize(
		/*maximum number of slices*/ 9
		/*max tris per slice*/ * 2
		/*max indicies per tri*/ * 3
	);
	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, graphics::MemoryUsage::eGPUOnly)
		.setSize(this->mIndices.size() * sizeof(ui16));
}

Image::Image(Image&& other)
{
	*this = std::move(other);
}

Image& Image::operator=(Image &&other)
{
	this->mPadding = other.mPadding;
	this->mSubSize = other.mSubSize;
	this->mSlicing = other.mSlicing;

	this->mpResource = other.mpResource;
	other.mpResource.reset();

	this->mVertices = other.mVertices;
	this->mIndices = other.mIndices;
	this->mVertexBuffer = std::move(other.mVertexBuffer);
	this->mIndexBuffer = std::move(other.mIndexBuffer);
	
	return *this;
}

Image::~Image()
{
	this->releaseGraphics();
}

void Image::releaseGraphics()
{
	this->mVertexBuffer.destroy();
	this->mIndexBuffer.destroy();
	this->mpResource.reset();
}

Image& Image::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	Widget::setDevice(device);
	this->mVertexBuffer.setDevice(device);
	this->mIndexBuffer.setDevice(device);
	return *this;
}

Image& Image::setResolution(ui::Resolution const& resolution)
{
	Widget::setResolution(resolution);
	return *this;
}

Image& Image::setAnchorParent(std::weak_ptr<ui::Widget> parent) { Widget::setAnchorParent(parent); return *this; }
Image& Image::setAnchor(math::Vector2 const& anchor) { Widget::setAnchor(anchor); return *this; }
Image& Image::setPivot(math::Vector2 const& pivot) { Widget::setPivot(pivot); return *this; }
Image& Image::setPosition(math::Vector2Int const& points) { Widget::setPosition(points); return *this; }
Image& Image::setSize(math::Vector2UInt const& points) { Widget::setSize(points); return *this; }
Image& Image::setZLayer(ui32 z) { Widget::setZLayer(z); return *this; }

Image& Image::setResource(std::weak_ptr<ui::ImageResource> const& resource)
{
	this->mpResource = resource;
	return *this;
}

Image& Image::setColor(math::Vector4 const& color)
{
	this->mColor = color;
	return *this;
}

Image& Image::setTexturePadding(math::Vector2UInt const& paddingInPixels)
{
	this->mPadding = paddingInPixels;
	return *this;
}

Image& Image::setTextureSubsize(math::Vector2UInt const& sizeInPixels)
{
	this->mSubSize = sizeInPixels;
	return *this;
}

Image& Image::setTextureSlicing(std::array<Slice, 4> const& slices)
{
	this->mSlicing = slices;
	return *this;
}

Widget& Image::create()
{
	this->mVertexBuffer.create();
	this->mIndexBuffer.create();
	return *this;
}

Widget& Image::commit(graphics::CommandPool* transientPool)
{
	assert(!this->mpResource.expired());

	this->mVertices.clear();
	this->mIndices.clear();
	
	auto pos = this->getTopLeftPositionOnScreen();
	auto size = this->getSizeOnScreen();
	auto const imageSize = this->mpResource.lock()->size().toFloat();

	struct Coordinate
	{
		f32 pos;
		f32 texCoord;
	};

	/*
		Lists of 1-dimensional coordinates where
		idx:0 is the coordinates along the x-axis
		idx:1 is the coordinates along the y-axis
	*/
	auto coords = std::array<std::vector<Coordinate>, 2>();
	
	auto left = Coordinate{ pos.x(), this->mPadding.x() / imageSize.x() };
	auto right = Coordinate{ pos.x() + size.x(), (this->mPadding.x() + this->mSubSize.x()) / imageSize.x() };
	auto top = Coordinate{ pos.y(), this->mPadding.y() / imageSize.y() };
	auto bottom = Coordinate{ pos.y() + size.y(), (this->mPadding.y() + this->mSubSize.y()) / imageSize.y() };

	coords[0].push_back(left);
	coords[1].push_back(top);
	for (ui8 idx = 0; idx < (ui8)this->mSlicing.size(); ++idx)
	{
		if (this->mSlicing[idx].points == 0) continue;

		auto axisIdx = idx / 2; // 0 for left/right, 1 for top/bottom
				
		auto sliceOffsetOnScreen = this->resolution().pointsToScreenSpace(
			math::Vector2Int(this->mSlicing[idx].points)
		);
		auto tex = math::Vector2UInt(this->mSlicing[idx].pixels);

		// If its the end of the axis (right or bottom)
		// 0 for left/top, 1 for right/bottom
		if (idx % 2 == 1)
		{
			sliceOffsetOnScreen = size - sliceOffsetOnScreen;
			tex = this->mSubSize - tex;
		}

		auto posOnScreen = pos + sliceOffsetOnScreen;
		auto texCoordNormalized = (this->mPadding + tex).toFloat() / imageSize;
		coords[axisIdx].push_back({
			posOnScreen[axisIdx], texCoordNormalized[axisIdx]
		});
	}
	coords[0].push_back(right);
	coords[1].push_back(bottom);

	auto vertexIndices = math::Matrix<ui16, 4, 4>(-1);
	for (ui8 idxColCoord = 0; idxColCoord < (ui8)coords[1].size(); ++idxColCoord)
	{
		for (ui8 idxRowCoord = 0; idxRowCoord < (ui8)coords[0].size(); ++idxRowCoord)
		{
			vertexIndices[idxRowCoord][idxColCoord] = ui16(this->mVertices.size());
			this->mVertices.push_back({
				{ coords[0][idxRowCoord].pos, coords[1][idxColCoord].pos, 0 },
				{ coords[0][idxRowCoord].texCoord, coords[1][idxColCoord].texCoord },
				this->mColor,
			});
		}
	}

	for (ui8 idxRowCoordA = 0; idxRowCoordA < ui8(coords[0].size()) - 1; ++idxRowCoordA)
	{
		ui8 idxRowCoordB = idxRowCoordA + 1;
		for (ui8 idxColCoordA = 0; idxColCoordA < ui8(coords[1].size()) - 1; ++idxColCoordA)
		{
			ui8 idxColCoordB = idxColCoordA + 1;
			this->pushTri({
				vertexIndices[idxRowCoordA][idxColCoordA],
				vertexIndices[idxRowCoordB][idxColCoordA],
				vertexIndices[idxRowCoordB][idxColCoordB]
			});
			this->pushTri({
				vertexIndices[idxRowCoordB][idxColCoordB],
				vertexIndices[idxRowCoordA][idxColCoordB],
				vertexIndices[idxRowCoordA][idxColCoordA]
			});
		}
	}

	this->mVertexBuffer.writeBuffer(transientPool, 0, this->mVertices);
	this->mIndexBuffer.writeBuffer(transientPool, 0, this->mIndices);
	return *this;
}

void Image::pushTri(math::Vector3UInt const& indices)
{
	this->mIndices.push_back(indices[0]);
	this->mIndices.push_back(indices[1]);
	this->mIndices.push_back(indices[2]);
}

void Image::bind(graphics::Command *command, std::shared_ptr<graphics::Pipeline> pipeline)
{
	OPTICK_EVENT();
	command->bindDescriptorSets(pipeline, { &this->mpResource.lock()->descriptor() });
	command->bindVertexBuffers(0, { &this->mVertexBuffer });
	command->bindIndexBuffer(0, &this->mIndexBuffer, vk::IndexType::eUint16);
}

void Image::record(graphics::Command *command)
{
	OPTICK_EVENT();
	command->draw(0, (ui32)this->mIndices.size(), 0, 0, 1);
}
