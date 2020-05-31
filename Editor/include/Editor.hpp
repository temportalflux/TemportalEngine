#pragma once

#include "TemportalEnginePCH.hpp"

#include "gui/MainDockspace.hpp"
#include "asset/Project.hpp"

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

	Editor();
	~Editor();

	bool setup();
	void run();

	bool hasProject() const;
	void setProject(asset::AssetPtrStrong asset);
	asset::ProjectPtrStrong getProject();

private:
	engine::Engine *mpEngine;

	gui::MainDockspace mDockspace;

	// The project that the editor is currently operating on
	asset::ProjectPtrStrong mpProject;

	void initializeRenderer(graphics::VulkanRenderer *pRenderer);

};
