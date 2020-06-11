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

	void addGui(std::string id, std::shared_ptr<gui::IGui> gui);
	std::shared_ptr<gui::IGui> removeGui(std::string id);

	void onInputEvent(void* evt) override;

	void drawFrame() override;

private:
	// TODO: Create wrapper inside graphics namespace
	vk::UniqueDescriptorPool mDescriptorPool;
	std::vector<graphics::ImGuiFrame> mGuiFrames;

	vk::UniqueDescriptorPool createDescriptorPoolImgui();
	void submitFonts();

	std::unordered_map<std::string, std::shared_ptr<gui::IGui>> mGuis;
	std::vector<std::string> mGuisToRemove;

	void startGuiFrame();
	void makeGui();
	void endGuiFrame();

protected:
	void createCommandObjects() override {}
	void destroyCommandObjects() override {}
	void createInputBuffers(ui64 vertexBufferSize, ui64 indexBufferSize) override {}
	void destroyInputBuffers() override {}
	void createUniformBuffers() override {}
	void destroyUniformBuffers() override {}
	void updateUniformBuffer(ui32 idxImageView) override {}
	void createDescriptorPool() override {}
	void destroyDescriptorPool() override {}

	void createFrames(uSize viewCount) override;
	uSize getNumberOfFrames() const override;
	graphics::Frame* getFrameAt(uSize idx) override;
	void destroyFrames() override;

	void render() override;

};

NS_END
