#include "graphics/PhysicalDevicePreference.hpp"

#include "graphics/PhysicalDevice.hpp"
#include "graphics/SwapChainSupport.hpp"
#include "graphics/QueueFamilyGroup.hpp"

#include <unordered_set>

using namespace graphics;

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaDeviceType(PhysicalDeviceType deviceType, IndividualScore score)
{
	mDeviceType.push_back({ score, deviceType });
	return *this;
}

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaDeviceExtension(PhysicalDeviceExtension::Type extensionName, IndividualScore score)
{
	mDeviceExtensions.push_back({ score, extensionName });
	return *this;
}

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaDeviceFeature(DeviceFeature feature, IndividualScore score)
{
	mFeatures.push_back({ score, feature });
	return *this;
}

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaQueueFamily(QueueFamily queueFamily, IndividualScore score)
{
	mQueueFamilies.push_back({ score, queueFamily });
	return *this;
}

PhysicalDevicePreference& PhysicalDevicePreference::addCriteriaSwapChain(SwapChainSupportType optionType, IndividualScore score)
{
	mSwapChain.push_back({ score, optionType });
	return *this;
}

std::optional<ui32> PhysicalDevicePreference::scoreDevice(graphics::PhysicalDevice const *pDevice)
{
	std::sort(this->mDeviceType.begin(), this->mDeviceType.end());
	std::sort(this->mDeviceExtensions.begin(), this->mDeviceExtensions.end());
	std::sort(this->mFeatures.begin(), this->mFeatures.end());
	std::sort(this->mQueueFamilies.begin(), this->mQueueFamilies.end());
	std::sort(this->mSwapChain.begin(), this->mSwapChain.end());

	// Defines if the scoring should examine all preferences, even if required ones have not been found (prevents returning early).
	bool bExamineAllPreferences = false;
	// True if all required preferences have been found.
	bool bFoundAllReqiuredPreferences = true;
	// The total score of the device.
	ui32 score = 0;
			
	auto deviceProperties = pDevice->getProperties();
	for (auto& prefDeviceType : this->mDeviceType)
	{
		if (!prefDeviceType.scoreAgainst(
			prefDeviceType.doesCriteriaMatch(EPhysicalDeviceType(deviceProperties.deviceType)),
			score
		))
		{
			bFoundAllReqiuredPreferences = false;
			if (!bExamineAllPreferences) return std::nullopt;
		}
	}

	// Determine if all required extensions are supported, and adding the score of those that are optional to `score`.
	auto supportedExtensions = pDevice->getSupportedExtensionNames();
	for (auto& prefExtension : mDeviceExtensions)
	{
		if (!prefExtension.scoreAgainst(supportedExtensions.count(prefExtension.value) != 0, score))
		{
			bFoundAllReqiuredPreferences = false;
			if (!bExamineAllPreferences) return std::nullopt;
		}
	}

	auto features = pDevice->getFeatures();
	for (auto& prefFeature : this->mFeatures)
	{
		if (!prefFeature.scoreAgainst(graphics::hasFeature(&features, prefFeature.value), score))
		{
			bFoundAllReqiuredPreferences = false;
			if (!bExamineAllPreferences) return std::nullopt;
		}
	}

	auto supportedQueueFamilyGroup = pDevice->queryQueueFamilyGroup();
	for (auto& prefQueueFamily : this->mQueueFamilies)
	{
		if (!prefQueueFamily.scoreAgainst(supportedQueueFamilyGroup.hasQueueFamily(prefQueueFamily.value), score))
		{
			bFoundAllReqiuredPreferences = false;
			if (!bExamineAllPreferences) return std::nullopt;
		}
	}

	auto swapChainSupport = pDevice->querySwapChainSupport();
	for (auto& prefSwapChain : this->mSwapChain)
	{
		if (!prefSwapChain.scoreAgainst(graphics::hasSupport(&swapChainSupport, prefSwapChain.value), score))
		{
			bFoundAllReqiuredPreferences = false;
			if (!bExamineAllPreferences) return std::nullopt;
		}
	}

	if (bExamineAllPreferences && !bFoundAllReqiuredPreferences)
	{
		return std::nullopt;
	}
	return score;
}

bool PhysicalDevicePreference::operator==(PhysicalDevicePreference const& other) const
{
	return
		mDeviceType == other.mDeviceType
		&& mDeviceExtensions == other.mDeviceExtensions
		&& mFeatures == other.mFeatures
		&& mQueueFamilies == other.mQueueFamilies
		&& mSwapChain == other.mSwapChain
		;
}

bool PhysicalDevicePreference::operator!=(PhysicalDevicePreference const& other) const
{
	return !(*this == other);
}
