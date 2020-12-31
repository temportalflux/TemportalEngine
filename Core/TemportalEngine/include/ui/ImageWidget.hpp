#pragma once

#include "ui/Widget.hpp"

#include "graphics/Buffer.hpp"
#include "graphics/Descriptor.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageView.hpp"

NS_UI

class ImageResource
{

public:
	ImageResource();
	ImageResource(ImageResource const& other) = delete;
	ImageResource(ImageResource&& other);
	ImageResource& operator=(ImageResource &&other);
	~ImageResource();

	ImageResource& setTexturePixels(std::vector<ui8> const& pixels);
	ImageResource& setTextureSize(math::Vector2UInt const& sizeInPixels);

	ImageResource& setDevice(std::weak_ptr<graphics::GraphicsDevice> device);

	/**
	 * Creates the graphics image for the provided pixels and size.
	 */
	ImageResource& create(graphics::CommandPool* transientPool);
	ImageResource& createDescriptor(graphics::DescriptorLayout *layout, graphics::DescriptorPool *descriptorPool);
	ImageResource& attachWithSampler(graphics::ImageSampler *sampler);
	void release();

	math::Vector2UInt const& size() const { return this->mImageSize; }
	graphics::DescriptorSet const& descriptor() const { return this->mDescriptor; }

private:
	/**
	 * The pixels of the image in 8-bit SRGB.
	 */
	std::vector<ui8> mPixels;
	math::Vector2UInt mImageSize;
	graphics::Image mImage;
	graphics::ImageView mView;
	graphics::DescriptorSet mDescriptor;

};

class Image : public ui::Widget
{
public:
	struct Vertex
	{
		math::Vector3Padded position;
		math::Vector2Padded textureCoordinate;
		math::Vector4 color;
	};

	struct Slice
	{
		/**
		 * The number of points in the screen this slice takes up.
		 */
		ui32 points;
		/**
		 * The number of pixels in the image this slice takes up.
		 */
		ui32 pixels;
	};

	Image();
	Image(Image const& other) = delete;
	Image(Image&& other);
	Image& operator=(Image &&other);
	~Image();

	Image& setDevice(std::weak_ptr<graphics::GraphicsDevice> device);
	Image& setResolution(ui::Resolution const& resolution);

	Image& setParent(std::weak_ptr<ui::Widget> parent);
	Image& setAnchor(math::Vector2 const& anchor);
	Image& setPivot(math::Vector2 const& pivot);
	Image& setPosition(math::Vector2Int const& points);
	Image& setSize(math::Vector2UInt const& points);
	Image& setFillWidth(bool bFill);
	Image& setFillHeight(bool bFill);
	Image& setZLayer(ui32 z);

	Image& setResource(std::weak_ptr<ui::ImageResource> const& resource);
	Image& setColor(math::Vector4 const& color);
	Image& setTexturePadding(math::Vector2UInt const& paddingInPixels);
	Image& setTextureSubsize(math::Vector2UInt const& sizeInPixels);

	Image& setTextureSlicing(std::array<Slice, 4> const& slices);

	/**
	 * Creates the buffer objects for the current image.
	 */
	Widget& create() override;

	/**
	 * Rebuilds the vertex and index buffers and commits them to the graphics object representations.
	 */
	Widget& commit(graphics::CommandPool* transientPool) override;

	void releaseGraphics();
	void bind(graphics::Command *command, std::shared_ptr<graphics::Pipeline> pipeline) override;
	void record(graphics::Command *command) override;

private:
	
	math::Vector4 mColor;

	/**
	 * The amount of pixels in the image to ignore from the left & top
	 * (before determining slicing).
	 */
	math::Vector2UInt mPadding;
	/**
	 * The size of the subimage (before slicing).
	 * Used with `mPadding` to determine the inner extents.
	 */
	std::optional<math::Vector2UInt> mSubSize;

	/**
	 * The amount of pixels from the padding at which the image is 9-sliced.
	 * 0: left
	 * 1: right
	 * 2: top
	 * 3: bottom
	 */
	std::array<Slice, 4> mSlicing;

	std::weak_ptr<ui::ImageResource> mpResource;

	std::vector<Vertex> mVertices;
	std::vector<ui16> mIndices;
	graphics::Buffer mVertexBuffer;
	graphics::Buffer mIndexBuffer;

	void pushTri(math::Vector3UInt const& indices);

};

NS_END
