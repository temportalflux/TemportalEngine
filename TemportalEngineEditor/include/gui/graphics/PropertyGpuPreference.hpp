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

	static bool renderPrefDeviceType(Preference<EDeviceType::Enum> &value);
	static bool renderPrefExtension(Preference<EExtension::Type> &value);
	static bool renderPrefFeature(Preference<EFeature::Enum> &value);
	static bool renderPrefQueueFam(Preference<EQueueFamily::Enum> &value);
	static bool renderPrefSwapChain(Preference<ESwapChain::Enum> &value);

};

NS_END
