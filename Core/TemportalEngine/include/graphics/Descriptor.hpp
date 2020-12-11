#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/types.hpp"

NS_GRAPHICS
class Buffer;
class GraphicsDevice;
class ImageView;
class ImageSampler;
class DescriptorPool;

struct DescriptorBinding
{
	std::string id;
	graphics::DescriptorType type;
	graphics::ShaderStage stage;
	ui32 count;
};

class DescriptorSet
{
	friend class DescriptorLayout;

public:
	DescriptorSet();
	DescriptorSet(DescriptorSet &&other);
	DescriptorSet& operator=(DescriptorSet &&other);
	~DescriptorSet();

	DescriptorSet& attach(
		std::string const &id, graphics::Buffer *buffer
	);

	DescriptorSet& attach(
		std::string const &id,
		ImageLayout const layout, ImageView *view, ImageSampler *sampler
	);

	DescriptorSet& writeAttachments();

	void* get() const;

private:

	DescriptorSet(
		std::weak_ptr<GraphicsDevice> device,
		std::vector<DescriptorBinding> const& bindings,
		/*vk::DescriptorSet*/ void* internalSet
	);

	std::weak_ptr<GraphicsDevice> mpDevice;
	std::vector<DescriptorBinding> mBindings;

	/*vk::DescriptorSet*/ void* mInternal;

	struct WriteInstruction
	{

		uIndex idxDescriptor;

		union
		{
			struct
			{
				Buffer* buffer; // 32/64 bit int (ptr)
				uIndex offset; // 32/64 bit int
				uSize size; // 32/64 bit int
			} buffer;
			struct
			{
				ImageView* view; // 32/64 bit int (ptr)
				ImageSampler* sampler; // 32/64 bit int (ptr)
				ImageLayout layout; // ui8 (1 byte)
			} image;
		} writeInfo;

	};
	std::vector<WriteInstruction> mWriteInstructions;

	std::optional<uIndex> findBinding(std::string const& id) const;

};

class DescriptorLayout
{
public:
	DescriptorLayout();
	DescriptorLayout(DescriptorLayout &&other);
	DescriptorLayout& operator=(DescriptorLayout &&other);
	~DescriptorLayout();

	DescriptorLayout& setDevice(std::weak_ptr<GraphicsDevice> device);

	// Sets the total number of bindings
	DescriptorLayout& setBindingCount(uSize count);

	DescriptorLayout& setBinding(
		uIndex const idx, std::string const &id,
		graphics::DescriptorType const type,
		graphics::ShaderStage const shaderStage,
		ui32 const& count
	);

	DescriptorLayout& create();
	void* get() const;
	void invalidate();

	DescriptorLayout& createSet(DescriptorPool *pool, DescriptorSet &outSet);
	DescriptorLayout& createSets(DescriptorPool *pool, std::vector<DescriptorSet> &outSets);

private:
	std::weak_ptr<GraphicsDevice> mpDevice;

	// The actual descriptor bindings for how the descriptors attach to parts of the pipeline
	std::vector<DescriptorBinding> mBindings;

	// The layout created from the bindings list and used to create the sets
	/*vk::DescriptorSetLayout*/ void* mInternal;

};

class DescriptorSetPool : public std::enable_shared_from_this<DescriptorSetPool>
{

public:
	struct Handle
	{
		friend class DescriptorSetPool;
		Handle();
		~Handle();
		DescriptorSet& get() const;
		DescriptorSet& operator*() const;
		void destroy();
	private:
		std::weak_ptr<DescriptorSetPool> mpPool;
		uIndex mIdxSet;
		Handle(std::weak_ptr<DescriptorSetPool> pool, uIndex idxSet);
	};

	DescriptorSetPool(DescriptorPool *descriptorPool);

	DescriptorLayout& layout();

	Handle create();
	DescriptorSet& get(uIndex const& idxSet);
	void destroy(Handle const& handle);

private:
	DescriptorPool *mpDescriptorPool;
	DescriptorLayout mLayout;
	std::vector<DescriptorSet> mSets;
	std::set<uIndex> mUnusedSetIndices;

};

NS_END
