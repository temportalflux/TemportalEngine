#pragma once

#include "CoreInclude.hpp"
#include "asset/TypedAssetPath.hpp"
#include "ui/Core.hpp"
#include "ui/ImageWidget.hpp"

FORWARD_DEF(NS_ASSET, class Texture);

NS_UI

extern asset::TypedAssetPath<asset::Texture> ASSET_IMG_BACKGROUND;
ui::Image& createMenuBackground(ui::Image& img, ui32 borderWidthInPoints);

extern asset::TypedAssetPath<asset::Texture> ASSET_IMG_SLOT;
ui::Image& createSlotImage(ui::Image& img, ui32 borderWidthInPoints);

NS_END
