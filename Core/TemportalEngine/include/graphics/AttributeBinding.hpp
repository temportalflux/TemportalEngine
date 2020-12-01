#pragma once

#include "TemportalEnginePCH.hpp"

NS_GRAPHICS

class AttributeBinding
{
	friend class Pipeline;

public:
	// Reflection of `vk::VertexInputRate`
	enum class Rate
	{
		eVertex = 0,
		eInstance = 1,
	};

	struct Attribute
	{

		/**
		 * The slot the attribute is bound to.
		 * `layout(location = slot) vec3 position`
		 */
		ui8 slot;

		/**
		 * The format of the attribute.
		 * See `vk::Format`
		 */
		ui32 format;

		/**
		 * The amount of bytes this attribute is from the start of the binding.
		 */
		ui32 offset;

	};

	AttributeBinding() = default;
	AttributeBinding(Rate inputRate) : mInputRate(inputRate), mSize(0) {}

	template <typename T>
	AttributeBinding& setStructType()
	{
		this->mSize = (ui32)sizeof(T);
		return *this;
	}

	AttributeBinding& addAttribute(Attribute attrib)
	{
		this->mAttributes.push_back(attrib);
		return *this;
	}

private:

	/**
	 * The size of the vertex attribute structure.
	 */
	ui32 mSize;
	/**
	 * 0 = per vertex, 1 = per instance
	 * See `vk::VertexInputRate`
	 */
	Rate mInputRate;

	std::vector<Attribute> mAttributes;

};

NS_END
