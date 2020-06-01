#pragma once

#include "TemportalEnginePCH.hpp"

#include "gui/MainDockspace.hpp"
#include "asset/Project.hpp"

#include <unordered_map>

class Window;

NS_MEMORY
class MemoryChunk;
NS_END

NS_ENGINE
class Engine;
NS_END

NS_GRAPHICS
class VulkanRenderer;
NS_END

class Editor
{

public:
	static Editor* EDITOR;

	Editor(std::shared_ptr<memory::MemoryChunk> mainMemory, std::unordered_map<std::string, ui64> memoryChunkSizes);
	~Editor();

	bool setup();
	void run();

	bool hasProject() const;
	void setProject(asset::AssetPtrStrong asset);
	asset::ProjectPtrStrong getProject();

private:
	std::shared_ptr<engine::Engine> mpEngine;
	std::shared_ptr<Window> mpWindow;

	gui::MainDockspace mDockspace;

	// The project that the editor is currently operating on
	asset::ProjectPtrStrong mpProject;

	void initializeRenderer(graphics::VulkanRenderer *pRenderer);

};
