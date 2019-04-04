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
class SharedPtr;

template <typename TValue>
using SharedPtrOpt = std::optional<SharedPtr<TValue>>;

template <typename TValue>
class TEMPORTALENGINE_API SharedPtr
{
	
private:
	TValue* mpRaw;
	PtrDealloc mfDealloc;

public:

	SharedPtr()
		: mpRaw(nullptr)
		, mfDealloc(nullptr)
	{
	}

	SharedPtr(TValue* raw, PtrDealloc dealloc)
		: mpRaw(raw)
		, mfDealloc(dealloc)
	{
	}

	~SharedPtr()
	{
		mpRaw->TValue::~TValue();
		mfDealloc((void**)&mpRaw);
		mpRaw = nullptr;
	}

	TValue* operator->() const
	{
		return mpRaw;
	}

};

template <typename TAlloc, typename... TArgs>
static SharedPtrOpt<TAlloc> NewUnique(TArgs... args)
{
	engine::Engine* pEngine;
	if (engine::Engine::GetChecked(pEngine))
	{
		TAlloc* rawPtr = (TAlloc*)pEngine->allocRaw(sizeof(TAlloc));
		if (rawPtr == nullptr) return std::nullopt;
		new (rawPtr) TAlloc(args...);
		auto dealloc = std::bind(&engine::Engine::deallocRaw, pEngine, std::placeholders::_1);
		auto uniquePtr = SharedPtr<TAlloc>(rawPtr, dealloc);
		return std::make_optional(uniquePtr);
	}
	return std::nullopt;
}

NS_END

#pragma warning(pop)
#endif