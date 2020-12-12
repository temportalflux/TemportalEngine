#pragma once

#include "TemportalEnginePCH.hpp"

template <typename TValue>
class DynamicHandle;

template <typename TValue>
class IDynamicHandleOwner : public std::enable_shared_from_this<IDynamicHandleOwner<TValue>>
{
public:
	virtual DynamicHandle<TValue> createHandle() = 0;
	virtual TValue* get(uIndex const& idx) = 0;
	virtual void destroyHandle(uIndex const& idx) = 0;
};

template <typename TValue>
class DynamicHandle
{
	using TOwner = IDynamicHandleOwner<TValue>;

public:
	DynamicHandle() = default;
	DynamicHandle(std::weak_ptr<TOwner> owner, uIndex const& idx) : mpOwner(owner), mIdx(idx) {}
	DynamicHandle(DynamicHandle<TValue> &&other) { *this = std::move(other); }

	DynamicHandle& operator=(DynamicHandle<TValue> &&other)
	{
		this->mpOwner = other.mpOwner;
		other.mpOwner.reset();
		this->mIdx = other.mIdx;
		return *this;
	}

	~DynamicHandle() { destroy(); }

	bool isValid() const { return !this->mpOwner.expired(); }
	operator bool() const { return isValid(); }
	operator uIndex() const { return this->mIdx; }

	template <typename TOwnerReal>
	std::shared_ptr<TOwnerReal> owner() const { return std::reinterpret_pointer_cast<TOwnerReal>(this->mpOwner.lock()); }

	TValue* get() const
	{
		assert(!this->mpOwner.expired());
		return this->mpOwner.lock()->get(this->mIdx);
	}
	TValue* operator*() const { return get(); }

	void destroy()
	{
		if (this->isValid())
		{
			this->mpOwner.lock()->destroyHandle(this->mIdx);
			this->mpOwner.reset();
		}
	}

private:
	std::weak_ptr<TOwner> mpOwner;
	uIndex mIdx;
};
