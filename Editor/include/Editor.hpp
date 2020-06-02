#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/Project.hpp"
#include "commandlet/Commandlet.hpp"
#include "gui/MainDockspace.hpp"
#include "utility/StringUtils.hpp"

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

	Editor(std::unordered_map<std::string, ui64> memoryChunkSizes);
	~Editor();

	void registerCommandlet(std::shared_ptr<editor::Commandlet> cmdlet);
	bool setup(utility::ArgumentMap args);
	void run(utility::ArgumentMap args);

	bool hasProject() const;
	void setProject(asset::AssetPtrStrong asset);
	asset::ProjectPtrStrong getProject();

private:
	bool mbShouldRender;

	std::shared_ptr<engine::Engine> mpEngine;

	std::unordered_map<std::string, std::shared_ptr<editor::Commandlet>> mCommandlets;

	std::shared_ptr<Window> mpWindow;
	gui::MainDockspace mDockspace;

	// The project that the editor is currently operating on
	asset::ProjectPtrStrong mpProject;

	void initializeRenderer(graphics::VulkanRenderer *pRenderer);
	void registerAllCommandlets();

};
