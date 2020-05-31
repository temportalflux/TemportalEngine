#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/VulkanRenderer.hpp"
#include "graphics/Surface.hpp"
#include "graphics/ImGuiFrame.hpp"

#include <vulkan/vulkan.hpp>

NS_GUI
class IGui;
NS_END

NS_GRAPHICS
class VulkanInstance;
class Surface;

class ImGuiRenderer : public VulkanRenderer
{

public:
	ImGuiRenderer(VulkanInstance *pInstance, Surface &surface);
	
	void initializeDevices() override;
	void finalizeInitialization() override;
	void invalidate() override;

	void addGui(gui::IGui *gui);
	void removeGui(gui::IGui *gui);

	void onInputEvent(void* evt) override;

	void drawFrame() override;

private:
	// TODO: Create wrapper inside graphics namespace
	vk::UniqueDescriptorPool mDescriptorPool;
	std::vector<graphics::ImGuiFrame> mGuiFrames;

	vk::UniqueDescriptorPool createDescriptorPool();
	void submitFonts();

	std::unordered_set<gui::IGui*> mGuis;
	void startGuiFrame();
	void makeGui();
	void endGuiFrame();

protected:
	void createCommandObjects() override {}
	void destroyCommandObjects() override {}
	void createInputBuffers(ui32 bufferSize) override {}
	void destroyInputBuffers() override {}

	void createFrames(uSize viewCount) override;
	uSize getNumberOfFrames() const override;
	graphics::Frame* getFrameAt(uSize idx) override;
	void destroyFrames() override;

	void render() override;

};

NS_END
