#pragma once

#include "TemportalEnginePCH.hpp"

#include "cereal/optional.hpp"
#include "cereal/list.hpp"
#include "graphics/types.hpp"
#include "cereal/GraphicsFlags.hpp"

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

		bool isRequired() const { return !this->score; }

		bool isLessRequired(Preference<T> const &other) const
		{
			return !this->score.has_value() && other.score.has_value();
		}

		bool operator<(Preference<T> const &other) const
		{
			return this->isLessRequired(other) || std::less()(this->value, other.value);
		}

		bool operator==(Preference<T> const& other) const { return score == other.score && value == other.value; }
		bool operator!=(Preference<T> const& other) const { return !(*this == other); }

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
			archive(
				cereal::make_nvp("score", this->score),
				cereal::make_nvp("value", this->value)
			);
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(
				cereal::make_nvp("score", this->score),
				cereal::make_nvp("value", this->value)
			);
		}

	};

	typedef std::vector<Preference<PhysicalDeviceType>> ListDeviceType;
	typedef std::vector<Preference<PhysicalDeviceExtension::Type>> ListDeviceExtensions;
	typedef std::vector<Preference<DeviceFeature>> ListFeatures;
	typedef std::vector<Preference<QueueFamily>> ListQueueFamilies;
	typedef std::vector<Preference<SwapChainSupportType>> ListSwapChain;

public:
	PhysicalDevicePreference() = default;

	PhysicalDevicePreference& addCriteriaDeviceType(PhysicalDeviceType deviceType, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaDeviceExtension(PhysicalDeviceExtension::Type extensionName, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaDeviceFeature(DeviceFeature feature, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaQueueFamily(QueueFamily queueFamily, IndividualScore score = std::nullopt);
	PhysicalDevicePreference& addCriteriaSwapChain(SwapChainSupportType optionType, IndividualScore score = std::nullopt);
	
	std::optional<ui32> scoreDevice(graphics::PhysicalDevice const *pDevice);

	ListDeviceType& getDeviceTypes() { return this->mDeviceType; }
	ListDeviceType const& getDeviceTypes() const { return this->mDeviceType; }
	ListDeviceExtensions& getDeviceExtensions() { return this->mDeviceExtensions; }
	ListDeviceExtensions const& getDeviceExtensions() const { return this->mDeviceExtensions; }
	ListFeatures& getFeatures() { return this->mFeatures; }
	ListFeatures const& getFeatures() const { return this->mFeatures; }
	ListQueueFamilies& getQueueFamilies() { return this->mQueueFamilies; }
	ListQueueFamilies const& getQueueFamilies() const { return this->mQueueFamilies; }
	ListSwapChain& getSwapChain() { return this->mSwapChain; }
	ListSwapChain const& getSwapChain() const { return this->mSwapChain; }

	bool operator==(PhysicalDevicePreference const& other) const;
	bool operator!=(PhysicalDevicePreference const& other) const;

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(cereal::make_nvp("type", this->mDeviceType));
		archive(cereal::make_nvp("extensions", this->mDeviceExtensions));
		archive(cereal::make_nvp("features", this->mFeatures));
		archive(cereal::make_nvp("queueFamilies", this->mQueueFamilies));
		archive(cereal::make_nvp("swapChain", this->mSwapChain));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(cereal::make_nvp("type", this->mDeviceType));
		archive(cereal::make_nvp("extensions", this->mDeviceExtensions));
		archive(cereal::make_nvp("features", this->mFeatures));
		archive(cereal::make_nvp("queueFamilies", this->mQueueFamilies));
		archive(cereal::make_nvp("swapChain", this->mSwapChain));
	}

private:
	ListDeviceType mDeviceType;
	ListDeviceExtensions mDeviceExtensions;
	ListFeatures mFeatures;
	ListQueueFamilies mQueueFamilies;
	ListSwapChain mSwapChain;

};

NS_END
