#pragma once

#include "TemportalEnginePCH.hpp"

#include "cereal/list.hpp"
#include "cereal/optional.hpp"
#include "cereal/GraphicsFlags.hpp"
#include "graphics/types.hpp"
#include "utility/Flags.hpp"

NS_GRAPHICS

struct RPPhase
{
	struct Attachment
	{
		graphics::ImageFormatReferenceType::Enum formatType;
		graphics::SampleCount::Enum samples;
		graphics::AttachmentLoadOp::Enum generalLoadOp;
		graphics::AttachmentStoreOp::Enum generalStoreOp;
		graphics::AttachmentLoadOp::Enum stencilLoadOp;
		graphics::AttachmentStoreOp::Enum stencilStoreOp;

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("format", this->formatType));
			archive(cereal::make_nvp("samples", this->samples));
			archive(cereal::make_nvp("general-load", this->generalLoadOp));
			archive(cereal::make_nvp("general-store", this->generalStoreOp));
			archive(cereal::make_nvp("stencil-load", this->stencilLoadOp));
			archive(cereal::make_nvp("stencil-store", this->stencilStoreOp));
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
		}
	};

	std::string name;
	std::vector<Attachment> colorAttachments;
	std::optional<Attachment> depthAttachment;

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(cereal::make_nvp("name", this->name));
		archive(cereal::make_nvp("colorAttachments", this->colorAttachments));
		archive(cereal::make_nvp("depthAttachment", this->depthAttachment));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(cereal::make_nvp("name", this->name));
		archive(cereal::make_nvp("colorAttachments", this->colorAttachments));
		archive(cereal::make_nvp("depthAttachment", this->depthAttachment));
	}
};
struct RPDependency
{
	struct Item
	{
		std::optional<uIndex> phaseIndex;
		utility::Flags<graphics::PipelineStageFlags> stageMask;
		utility::Flags<graphics::AccessFlags> accessMask;

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(cereal::make_nvp("phase", this->phaseIndex));
			archive(cereal::make_nvp("stageMask", this->stageMask));
			archive(cereal::make_nvp("accessMask", this->accessMask));
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(cereal::make_nvp("phase", this->phaseIndex));
			archive(cereal::make_nvp("stageMask", this->stageMask));
			archive(cereal::make_nvp("accessMask", this->accessMask));
		}
	};

	/**
	 * The info about the phase that must happen before `depender`.
	 */
	Item dependee;
	/**
	 * The info about the phase that is depending on `dependee`.
	 */
	Item depender;

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(cereal::make_nvp("dependee", this->dependee));
		archive(cereal::make_nvp("depender", this->depender));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(cereal::make_nvp("dependee", this->dependee));
		archive(cereal::make_nvp("depender", this->depender));
	}
};

NS_END
