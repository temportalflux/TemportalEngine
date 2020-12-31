#pragma once

#include "TemportalEnginePCH.hpp"

#include "cereal/list.hpp"
#include "cereal/optional.hpp"
#include "cereal/GraphicsFlags.hpp"
#include "graphics/types.hpp"
#include "utility/Flags.hpp"

NS_GRAPHICS

struct RenderPassAttachment
{
	graphics::ImageFormatCategory formatType;
	graphics::SampleCount samples;
	graphics::AttachmentLoadOp generalLoadOp;
	graphics::AttachmentStoreOp generalStoreOp;
	graphics::AttachmentLoadOp stencilLoadOp;
	graphics::AttachmentStoreOp stencilStoreOp;
	graphics::ImageLayout initialLayout;
	graphics::ImageLayout finalLayout;

	bool operator==(RenderPassAttachment const& other) const
	{
		return formatType == other.formatType
			&& samples == other.samples
			&& generalLoadOp == other.generalLoadOp
			&& generalStoreOp == other.generalStoreOp
			&& stencilLoadOp == other.stencilLoadOp
			&& stencilStoreOp == other.stencilStoreOp
			&& initialLayout == other.initialLayout
			&& finalLayout == other.finalLayout;
	}
	bool operator!=(RenderPassAttachment const& other) const { return !(*this == other); }

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(cereal::make_nvp("format", this->formatType));
		archive(cereal::make_nvp("samples", this->samples));
		archive(cereal::make_nvp("general-load", this->generalLoadOp));
		archive(cereal::make_nvp("general-store", this->generalStoreOp));
		archive(cereal::make_nvp("stencil-load", this->stencilLoadOp));
		archive(cereal::make_nvp("stencil-store", this->stencilStoreOp));
		archive(cereal::make_nvp("layout-initial", this->initialLayout));
		archive(cereal::make_nvp("layout-final", this->finalLayout));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(cereal::make_nvp("format", this->formatType));
		archive(cereal::make_nvp("samples", this->samples));
		archive(cereal::make_nvp("general-load", this->generalLoadOp));
		archive(cereal::make_nvp("general-store", this->generalStoreOp));
		archive(cereal::make_nvp("stencil-load", this->stencilLoadOp));
		archive(cereal::make_nvp("stencil-store", this->stencilStoreOp));
		archive(cereal::make_nvp("layout-initial", this->initialLayout));
		archive(cereal::make_nvp("layout-final", this->finalLayout));
	}
};

struct RenderPassAttachmentReference
{
	uIndex attachment;
	graphics::ImageLayout layout;

	bool operator!=(RenderPassAttachmentReference const& other) const { return !((*this) == other); }
	bool operator==(RenderPassAttachmentReference const& other) const
	{ return this->attachment == other.attachment && this->layout == other.layout; }

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(cereal::make_nvp("attachment", this->attachment));
		archive(cereal::make_nvp("layout", this->layout));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(cereal::make_nvp("attachment", this->attachment));
		archive(cereal::make_nvp("layout", this->layout));
	}
};

struct RenderPassPhase
{
	std::vector<std::optional<RenderPassAttachmentReference>> colorAttachments;
	std::optional<RenderPassAttachmentReference> depthAttachment;

	bool operator!=(RenderPassPhase const& other) const { return !((*this) == other); }
	bool operator==(RenderPassPhase const& other) const
	{ return this->colorAttachments == other.colorAttachments && this->depthAttachment == other.depthAttachment; }

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(cereal::make_nvp("colorAttachments", this->colorAttachments));
		archive(cereal::make_nvp("depthAttachment", this->depthAttachment));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(cereal::make_nvp("colorAttachments", this->colorAttachments));
		archive(cereal::make_nvp("depthAttachment", this->depthAttachment));
	}
};

struct RenderPassDependency
{
	struct Item
	{
		std::optional<uIndex> phase;
		utility::Flags<graphics::PipelineStageFlags> stageMask;
		utility::Flags<graphics::AccessFlags> accessMask;

		bool operator!=(Item const& other) const { return !((*this) == other); }
		bool operator==(Item const& other) const
		{ return this->phase == other.phase && this->stageMask == other.stageMask && this->accessMask == other.accessMask; }

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("phase", this->phase));
			archive(cereal::make_nvp("stageMask", this->stageMask));
			archive(cereal::make_nvp("accessMask", this->accessMask));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("phase", this->phase));
			archive(cereal::make_nvp("stageMask", this->stageMask));
			archive(cereal::make_nvp("accessMask", this->accessMask));
		}
	};

	Item prev;
	Item next;

	bool operator!=(RenderPassDependency const& other) const { return !((*this) == other); }
	bool operator==(RenderPassDependency const& other) const
	{ return this->prev == other.prev && this->next == other.next; }

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(cereal::make_nvp("previous", this->prev));
		archive(cereal::make_nvp("next", this->next));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(cereal::make_nvp("previous", this->prev));
		archive(cereal::make_nvp("next", this->next));
	}
};

NS_END
