#pragma once

#include "TemportalEnginePCH.hpp"

template <typename TValue>
class DynamicHandle;

template <typename TValue>
class IDynamicHandleOwner
{
public:
	virtual DynamicHandle<TValue> createHandle() = 0;
	virtual TValue& get(uIndex const& idx) = 0;
	virtual void destroyHandle(uIndex const& idx) = 0;
};

template <typename TValue>
class DynamicHandle
{
	using TOwner = IDynamicHandleOwner<TValue>;

public:
	DynamicHandle() = default;
	DynamicHandle(std::weak_ptr<TOwner> owner, uIndex const& idx) : mpOwner(owner), mIdx(idx) {}
	~DynamicHandle() { destroy(); }

	TValue& get() const
	{
		assert(!this->mpOwner.expired());
		return this->mpOwner.lock()->get(this->mIdx);
	}

	TValue& operator*() const { return get(); }
	
	void destroy()
	{
		if (!this->mpOwner.expired())
		{
			this->mpOwner.lock()->destroyHandle(this->mIdx);
			this->mpOwner.reset();
		}
	}

private:
	std::weak_ptr<TOwner> mpOwner;
	uIndex mIdx;
};
