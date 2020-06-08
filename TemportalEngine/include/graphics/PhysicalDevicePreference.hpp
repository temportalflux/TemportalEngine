#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/SwapChainSupport.hpp"
#include "graphics/types.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class PhysicalDevice;

/**
	A set of scored preferences for determining the most applicable
	Physical Device/GPU for a pipeline.
*/
class PhysicalDevicePreference
{
public:
	typedef std::optional<ui8> IndividualScore;
	typedef std::optional<ui32> TotalScore;

	struct Preference
	{
		/**
			How important this preferences is compared to others.
			If this value is `std::nullopt`, then the preferences is required
			and any gpus/physical devices which do not meet this preferences
			are ignored.
		*/
		IndividualScore score;

		bool isScoreLessThan(Preference const &other) const
		{
			return !this->score.has_value() && other.score.has_value();
		}
	};

	struct PreferenceDeviceType : Preference
	{
		PhysicalDeviceProperties::Type::Enum type;

		bool operator<(PreferenceDeviceType const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->type, other.type);
		}
	};

	struct PreferenceExtension : Preference
	{
		PhysicalDeviceProperties::Extension::Type extensionName;

		bool operator<(PreferenceExtension const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->extensionName, other.extensionName);
		}
	};

	struct PreferenceFeature : Preference
	{
		graphics::PhysicalDeviceProperties::Feature::Enum feature;

		bool operator<(PreferenceFeature const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->feature, other.feature);
		}
	};

	struct PreferenceQueueFamily : Preference
	{
		QueueFamily::Enum queueFamily;

		bool operator<(PreferenceQueueFamily const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->queueFamily, other.queueFamily);
		}
	};

	struct PreferenceSwapChain : Preference
	{
		SwapChainSupportType::Enum supportType;

		bool operator<(PreferenceSwapChain const &other) const
		{
			return this->isScoreLessThan(other) || std::less()(this->supportType, other.supportType);
		}
	};

public:
	PhysicalDevicePreference() = default;

	PhysicalDevicePreference& addCriteriaDeviceType(PhysicalDeviceProperties::Type::Enum deviceType, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaDeviceExtension(PhysicalDeviceProperties::Extension::Type extensionName, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaDeviceFeature(PhysicalDeviceProperties::Feature::Enum feature, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaQueueFamily(QueueFamily::Enum queueFamily, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaSwapChain(SwapChainSupportType::Enum optionType, IndividualScore score = std::nullopt);
	
	TotalScore scoreDevice(graphics::PhysicalDevice const *pDevice) const;

private:
	std::set<PreferenceDeviceType> mDeviceType;
	std::set<PreferenceExtension> mDeviceExtensions;
	std::set<PreferenceFeature> mFeatures;
	std::set<PreferenceQueueFamily> mQueueFamilies;
	std::set<PreferenceSwapChain> mSwapChain;

};

NS_END
