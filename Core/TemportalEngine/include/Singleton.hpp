#pragma once

#include "TemportalEnginePCH.hpp"

template <typename T, typename... TInitArguments>
class Singleton : public std::enable_shared_from_this<T>
{
public:

	static std::shared_ptr<T> Create(TInitArguments... args)
	{
		assert(!T::gpInstance);
		T::gpInstance = std::make_shared<T>(args...);
		return T::Get();
	}

	static std::shared_ptr<T> Get()
	{
		return T::gpInstance;
	}

	static void Destroy()
	{
		assert(T::gpInstance && T::gpInstance.use_count() == 1);
		T::gpInstance.reset();
	}

private:
	static std::shared_ptr<T> gpInstance;
};
