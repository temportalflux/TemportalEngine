#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/Image.hpp"
#include "graphics/ImageView.hpp"
#include "graphics/ImageSampler.hpp"

#include <imgui.h>

NS_GRAPHICS
class ImGuiRenderer;

class ImGuiTexture : public std::enable_shared_from_this<ImGuiTexture>
{

public:
	ImGuiTexture(std::shared_ptr<ImGuiRenderer> renderer, ImTextureID textureId);
	~ImGuiTexture();
	
	ImTextureID const& id() const;
	graphics::Image& image();
	graphics::ImageView& view();
	graphics::ImageSampler& sampler();

	void create(std::vector<ui8> const& pixels);
	bool isValid() const;
	void invalidate();
	void releaseToPool();
	
private:
	std::weak_ptr<ImGuiRenderer> mpRenderer;

	ImTextureID mTextureId;
	bool bHasCreated;
	graphics::Image mImage;
	graphics::ImageView mView;
	graphics::ImageSampler mSampler;

};

NS_END
