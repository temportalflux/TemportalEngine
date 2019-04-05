#ifndef MEM_PTR_UNIQUE_HPP
#define MEM_PTR_UNIQUE_HPP
#pragma warning(push)
#pragma warning(disable:4251) // disable STL warnings in dll

#include "TemportalEnginePCH.hpp"

#include <functional>
#include <optional>

#include "Engine.hpp"

NS_MEMORY

using PtrDealloc = std::function<void(void**)>;

template <typename TValue>
class AllocatedPtr;

template <typename TValue>
using AllocatedPtrOpt = std::optional<AllocatedPtr<TValue>>;

template <typename TValue>
class TEMPORTALENGINE_API AllocatedPtr
{
	
private:
	bool mIsOwner;
	TValue* mpRaw;
	PtrDealloc mfDealloc;

public:

	AllocatedPtr()
		: mIsOwner(false)
		, mpRaw(nullptr)
		, mfDealloc(nullptr)
	{
	}

	AllocatedPtr(TValue* raw, PtrDealloc dealloc)
		: mIsOwner(true)
		, mpRaw(raw)
		, mfDealloc(dealloc)
	{
	}

	AllocatedPtr(const AllocatedPtr<TValue> &other)
	{
		mpRaw = other.mpRaw;
		mIsOwner = false;
	}

	~AllocatedPtr()
	{
		if (mIsOwner)
		{
			mpRaw->TValue::~TValue();
			mfDealloc((void**)&mpRaw);
			mpRaw = nullptr;
		}
	}

	TValue* operator->() const
	{
		return GetRaw();
	}

	TValue* GetRaw() const
	{
		return mpRaw;
	}

};

template <typename TAlloc, typename... TArgs>
static AllocatedPtrOpt<TAlloc> NewUnique(TArgs... args)
{
	engine::Engine* pEngine;
	if (engine::Engine::GetChecked(pEngine))
	{
		TAlloc* rawPtr = (TAlloc*)pEngine->allocRaw(sizeof(TAlloc));
		if (rawPtr == nullptr) return std::nullopt;
		new (rawPtr) TAlloc(args...);
		auto dealloc = std::bind(&engine::Engine::deallocRaw, pEngine, std::placeholders::_1);
		auto uniquePtr = AllocatedPtr<TAlloc>(rawPtr, dealloc);
		return std::make_optional(uniquePtr);
	}
	return std::nullopt;
}

NS_END

#pragma warning(pop)
#endif