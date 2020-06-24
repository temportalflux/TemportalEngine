#pragma once

#include "gui/IGui.hpp"

#include "logging/Logger.hpp"

#include <vector>
#include <array>
#include <string>

NS_GUI

class Log : public IGui
{

public:
	Log() = default;
	Log(std::string title);

	void onAddedToRenderer(graphics::ImGuiRenderer *pRenderer);
	void onRemovedFromRenderer(graphics::ImGuiRenderer *pRenderer);
	logging::LogSystem::Listener createLogListener();

	void renderView() override;

protected:
	i32 getFlags() const override;

private:
	logging::LogSystem::ListenerHandle mLogHandle;

	bool mbAutoScroll;
	std::vector<std::string> mLines;

	std::array<char, 64> mRawFilter;
	std::string mCurrentFilter;
	std::vector<std::string> mLinesFiltered;

	std::string makeFilter() const;
	bool matchesFilter(std::string const &item, std::string const&filter) const;

	void clear();
	void add(std::string timestamp, logging::ECategory category, std::string logName, std::string content);

	void updateFilter();

	void renderMenuBar();
	void renderFilterBar();
	void renderContent();

	/**
	 * Renders a subset of `lines` based on the height of the widget (controlled by ImGui).
	 */
	void renderLines(std::vector<std::string> const &lines);

};

NS_END
