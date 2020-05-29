#pragma once

#include "TemportalEnginePCH.hpp"

#include <string>

NS_ASSET

struct AssetId
{

	/**
	 * An identifier of 
	 */
	std::string Type;

	/**
	 * The unique path to the asset file relative to the asset directory.
	 * TODO: Where is the asset directory path stored?
	 */
	std::string Id;

};

NS_END
