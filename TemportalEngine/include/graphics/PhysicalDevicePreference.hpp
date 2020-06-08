#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/types.hpp"

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

	template <typename T>
	struct Preference
	{
		/**
			How important this preferences is compared to others.
			If this value is `std::nullopt`, then the preferences is required
			and any GPUs/physical devices which do not meet this preferences
			are ignored.
		*/
		IndividualScore score;
		T value;

		bool isLessRequired(Preference<T> const &other) const
		{
			return !this->score.has_value() && other.score.has_value();
		}

		bool operator<(Preference<T> const &other) const
		{
			return this->isLessRequired(other) || std::less()(this->value, other.value);
		}

		bool doesCriteriaMatch(T const other) const
		{
			return this->value == other;
		}

		/**
		 * Returns true only if the preference matches `other` or the preference is not required.
		 * `totalScore` will be added to by `score` if and only if the preference matches `other`.
		 */
		bool scoreAgainst(bool bMatches, ui32 &totalScore)
		{
			auto bRequired = !this->score.has_value();
			if (bMatches && !bRequired)
			{
				totalScore += this->score.value();
			}
			return bMatches || !bRequired;
		}

		template <typename Archive>
		void save(Archive &archive) const
		{

		}

		template <typename Archive>
		void load(Archive &archive)
		{

		}

	};

public:
	PhysicalDevicePreference() = default;

	PhysicalDevicePreference& addCriteriaDeviceType(PhysicalDeviceProperties::Type::Enum deviceType, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaDeviceExtension(PhysicalDeviceProperties::Extension::Type extensionName, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaDeviceFeature(PhysicalDeviceProperties::Feature::Enum feature, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaQueueFamily(QueueFamily::Enum queueFamily, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaSwapChain(SwapChainSupportType::Enum optionType, IndividualScore score = std::nullopt);
	
	std::optional<ui32> scoreDevice(graphics::PhysicalDevice const *pDevice);

	template <typename Archive>
	void save(Archive &archive) const
	{
		ui32 len;

		//len = (ui32)this->mDeviceType.size();
		//archive(len);
		//for (auto& pref : this->mDeviceType) archive(pref);

	}

	template <typename Archive>
	void load(Archive &archive)
	{
		ui32 len;

		//archive(len);
		//this->mDeviceType.resize(len);
		//for (auto& pref : this->mDeviceType) archive(pref);

	}

private:
	std::vector<Preference<PhysicalDeviceProperties::Type::Enum>> mDeviceType;
	std::vector<Preference<PhysicalDeviceProperties::Extension::Type>> mDeviceExtensions;
	std::vector<Preference<graphics::PhysicalDeviceProperties::Feature::Enum>> mFeatures;
	std::vector<Preference<QueueFamily::Enum>> mQueueFamilies;
	std::vector<Preference<SwapChainSupportType::Enum>> mSwapChain;

};

NS_END
