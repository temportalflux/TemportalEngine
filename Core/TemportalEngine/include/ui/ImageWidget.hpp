#pragma once

#include "ui/Widget.hpp"

#include "graphics/Buffer.hpp"
#include "graphics/Descriptor.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageView.hpp"

NS_UI

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

	Image& setRenderPosition(math::Vector2Int const& points);
	Image& setRenderSize(math::Vector2UInt const& points);

	Image& setTexturePixels(std::vector<ui8> const& pixels);
	Image& setTextureSize(math::Vector2UInt const& sizeInPixels);
	Image& setColor(math::Vector4 const& color);
	Image& setTexturePadding(math::Vector2UInt const& paddingInPixels);
	Image& setTextureSubsize(math::Vector2UInt const& sizeInPixels);

	Image& setTextureSlicing(std::array<Slice, 4> const& slices);

	/**
	 * Creates the graphics image for the provided pixels and size.
	 * Creates the buffer objects for the current image.
	 */
	Widget& create(graphics::CommandPool* transientPool) override;
	Widget& createDescriptor(graphics::DescriptorLayout *layout, graphics::DescriptorPool *descriptorPool) override;
	Widget& attachWithSampler(graphics::ImageSampler *sampler) override;

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
	math::Vector2UInt mSubSize;

	/**
	 * The amount of pixels from the padding at which the image is 9-sliced.
	 * 0: left
	 * 1: right
	 * 2: top
	 * 3: bottom
	 */
	std::array<Slice, 4> mSlicing;
	
	/**
	 * The pixels of the image in 8-bit SRGB.
	 */
	std::vector<ui8> mPixels;
	math::Vector2UInt mImageSize;
	graphics::Image mImage;
	graphics::ImageView mView;
	graphics::DescriptorSet mDescriptor;

	std::vector<Vertex> mVertices;
	std::vector<ui16> mIndices;
	graphics::Buffer mVertexBuffer;
	graphics::Buffer mIndexBuffer;

	void pushTri(math::Vector3UInt const& indices);

};

NS_END
