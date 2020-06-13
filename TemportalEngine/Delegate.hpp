#pragma once

#include "TemportalEnginePCH.hpp"

template<class TSignature>
class ExecuteDelegate
{
	typedef std::function<TSignature> TBinding;

public:

	void bind(TBinding binding)
	{
		this->mBinding = binding;
	}

	template <typename... TArgs>
	void execute(TArgs... args)
	{
		if (this->mBinding)
		{
			this->mBinding(args...);
		}
	}

	void clear()
	{
		this->mBinding = nullptr;
	}

private:
	TBinding mBinding;

};

template <class TSignature>
class BroadcastDelegate
{
	typedef std::function<TSignature> TBinding;
	typedef void* TKey;

public:

	bool hasBindingFor(TKey key)
	{
		return this->mBindings.find(key) != this->mBindings.end();
	}

	void bind(TKey key, TBinding binding)
	{
		assert(!this->hasBindingFor(key));
		this->mBindings.insert(std::make_pair(key, binding));
	}

	void unbind(TKey key)
	{
		assert(this->hasBindingFor(key));
		this->mBindings.erase(this->mBindings.find(key));
	}

	template <typename... TArgs>
	void broadcast(TArgs... args)
	{
		for (auto& [key, binding] : this->mBindings)
		{
			binding(args...);
		}
	}

private:
	/**
	 * A map keyed by object pointers to the binding they have added.
	 */
	std::unordered_map<TKey, TBinding> mBindings;

};
