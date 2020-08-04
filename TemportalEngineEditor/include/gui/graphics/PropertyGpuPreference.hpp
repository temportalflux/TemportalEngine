#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/PhysicalDevicePreference.hpp"

NS_GUI

class PropertyGpuPreference
{
	typedef graphics::PhysicalDevicePreference Internal;
	template <typename T>
	using Preference = graphics::PhysicalDevicePreference::Preference<T>;
	typedef graphics::PhysicalDeviceProperties::Type EDeviceType;
	typedef graphics::PhysicalDeviceProperties::Extension EExtension;
	typedef graphics::PhysicalDeviceProperties::Feature EFeature;
	typedef graphics::QueueFamily EQueueFamily;
	typedef graphics::SwapChainSupportType ESwapChain;

public:
	PropertyGpuPreference() = default;
	PropertyGpuPreference(Internal const &value);

	Internal& value() { return this->mInternal; }

	bool render(char const* titleId);

private:
	Internal mInternal;

	static bool renderPrefDeviceType(uIndex const& idx, Preference<EDeviceType::Enum> &value);
	static bool renderPrefExtension(uIndex const& idx, Preference<EExtension::Type> &value);
	static bool renderPrefFeature(uIndex const& idx, Preference<EFeature::Enum> &value);
	static bool renderPrefQueueFam(uIndex const& idx, Preference<EQueueFamily::Enum> &value);
	static bool renderPrefSwapChain(uIndex const& idx, Preference<ESwapChain::Enum> &value);

};

NS_END
