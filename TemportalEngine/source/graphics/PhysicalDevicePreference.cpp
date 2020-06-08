#include "graphics/PhysicalDevicePreference.hpp"

#include "graphics/PhysicalDevice.hpp"

#include <unordered_set>

using namespace graphics;

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaDeviceType(PhysicalDeviceProperties::Type::Enum deviceType, IndividualScore score)
{
	mDeviceType.insert({ score, deviceType });
	return *this;
}

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaDeviceExtension(PhysicalDeviceProperties::Extension::Type extensionName, IndividualScore score)
{
	mDeviceExtensions.insert({ score, extensionName });
	return *this;
}

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaDeviceFeature(PhysicalDeviceProperties::Feature::Enum feature, IndividualScore score)
{
	mFeatures.insert({ score, feature });
	return *this;
}

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaQueueFamily(QueueFamily::Enum queueFamily, IndividualScore score)
{
	mQueueFamilies.insert({ score, queueFamily });
	return *this;
}

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaSwapChain(SwapChainSupportType::Enum optionType, IndividualScore score)
{
	mSwapChain.insert({ score, optionType });
	return *this;
}

QueueFamilyGroup findQueueFamilies(vk::PhysicalDevice const &device, vk::SurfaceKHR const &surface)
{
	auto group = QueueFamilyGroup();
	auto familyProps = device.getQueueFamilyProperties();
	ui32 idxQueue = 0;
	for (auto& queueFamily : familyProps)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			group.idxGraphicsQueue = idxQueue;
		}

		if (device.getSurfaceSupportKHR(idxQueue, surface))
		{
			group.idxPresentationQueue = idxQueue;
		}

		if (group.hasFoundAllQueues()) break;
		idxQueue++;
	}
	return group;
}

PhysicalDevicePreference::TotalScore PhysicalDevicePreference::scoreDevice(graphics::PhysicalDevice const *pDevice) const
{
	// Defines if the scoring should examine all preferences, even if required ones have not been found (prevents returning early).
	bool bExamineAllPreferences = false;
	// True if all required preferences have been found.
	bool bFoundAllReqiuredPreferences = true;
	// The total score of the device.
	ui32 score = 0;

	// Updates the `score` and `bFoundAllReqiuredPreferences` based on if a preference is supported.
	// Returns `bExamineAllPreferences` if the preference is required and is not supported.
	// Otherwise returns true (required and supported or regardless if it is supported or not).
	auto scorePreference = [&](Preference pref, bool bIsSupported) {
		bool bIsRequired = !pref.score.has_value();
		if (bIsRequired && !bIsSupported)
		{
			// Extension is required and not found. Mark device as not applicable and early out if desired.
			bFoundAllReqiuredPreferences = false;
			return bExamineAllPreferences;
		}
		else if (!bIsRequired && bIsSupported)
		{
			score += pref.score.value();
		}
		return true;
	};

	auto deviceProperties = pDevice->getProperties();
	for (auto& prefDeviceType : this->mDeviceType)
	{
		if (!scorePreference(prefDeviceType,
			deviceProperties.deviceType == (vk::PhysicalDeviceType)prefDeviceType.type
		))
		{
			return std::nullopt;
		}
	}

	// Determine if all required extensions are supported, and adding the score of those that are optional to `score`.
	auto supportedExtensions = pDevice->getSupportedExtensionNames();
	for (auto& prefExtension : mDeviceExtensions)
	{
		if (!scorePreference(prefExtension,
			supportedExtensions.count(prefExtension.extensionName) != 0
		))
		{
			return std::nullopt;
		}
	}

	auto features = pDevice->getFeatures();
	for (auto& prefFeature : this->mFeatures)
	{
		if (!scorePreference(
			prefFeature,
			PhysicalDeviceProperties::Feature::hasFeature(&features, prefFeature.feature)
		))
		{
			return std::nullopt;
		}
	}

	auto supportedQueueFamilyGroup = pDevice->queryQueueFamilyGroup();
	for (auto& prefQueueFamily : this->mQueueFamilies)
	{
		if (!scorePreference(prefQueueFamily,
			supportedQueueFamilyGroup.hasQueueFamily(prefQueueFamily.queueFamily)
		))
		{
			return std::nullopt;
		}
	}

	auto swapChainSupport = pDevice->querySwapChainSupport();
	for (auto& prefSwapChain : this->mSwapChain)
	{
		if (!scorePreference(
			prefSwapChain,
			SwapChainSupportType::hasSupport(&swapChainSupport, prefSwapChain.supportType)
		))
		{
			return std::nullopt;
		}
	}

	if (bExamineAllPreferences && !bFoundAllReqiuredPreferences)
	{
		return std::nullopt;
	}
	return score;
}
