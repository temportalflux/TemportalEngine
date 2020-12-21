#pragma once

#include "TemportalEnginePCH.hpp"

NS_RENDER

template <typename TVertex>
class IModel
{

public:

	ui32 pushVertex(TVertex const& v)
	{
		auto i = (ui16)this->mVertices.size();
		this->mVertices.push_back(v);
		return i;
	}

	void pushIndex(ui32 const& i)
	{
		this->mIndicies.push_back(i);
	}

	void pushTri(math::Vector3UInt const& tri)
	{
		this->pushIndex(tri[0]);
		this->pushIndex(tri[1]);
		this->pushIndex(tri[2]);
	}

	uSize getVertexBufferSize() const
	{
		return (uSize)this->mVertices.size() * sizeof(TVertex);
	}

	uSize getIndexBufferSize() const
	{
		return (uSize)this->mIndicies.size() * sizeof(ui32);
	}

	std::vector<TVertex> const& vertices() const { return this->mVertices; }
	std::vector<ui32> const& indices() const { return this->mIndicies; }

private:
	std::vector<TVertex> mVertices;
	std::vector<ui32> mIndicies;

};

NS_END
