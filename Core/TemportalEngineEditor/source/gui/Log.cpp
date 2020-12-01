#include "gui/Log.hpp"

#include "Engine.hpp"
#include "utility/StringUtils.hpp"

#include <regex>
#include <imgui.h>
#include <cctype>

using namespace gui;

Log::Log(std::string title) : IGui(title)
{
	this->mbAutoScroll = true;
	this->mLines.reserve(256);
	this->mLinesFiltered.reserve(256);
	this->mRawFilter.fill('\0');
	this->mLogHandle = engine::Engine::LOG_SYSTEM.addListener(this->createLogListener());
}

Log::~Log()
{
	engine::Engine::LOG_SYSTEM.removeListener(this->mLogHandle);
}

logging::LogSystem::Listener Log::createLogListener()
{
	return std::bind(&Log::add, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
}

i32 Log::getFlags() const
{
	return ImGuiWindowFlags_MenuBar;
}

void Log::clear()
{
	this->mLines.clear();
	this->mLinesFiltered.clear();
}

void Log::add(std::string timestamp, logging::ECategory category, std::string logName, std::string content)
{
	std::string formatted = "[" + timestamp + "] [" + logging::LogSystem::getCategoryShortString(category) + "] " + logName + "> " + content;
	this->mLines.push_back(formatted);
	if (this->matchesFilter(formatted, this->mCurrentFilter))
	{
		this->mLinesFiltered.push_back(formatted);
	}
}

std::string Log::makeFilter() const
{
	return utility::createStringFromFixedArray(this->mRawFilter);
}

void Log::updateFilter()
{

	// TODO: Can optimize based on the change in the filter (if not regex and there are more characters, then we can just further prune the list of already filtered items)
	auto filter = this->makeFilter();

	this->mLinesFiltered.reserve(this->mLines.size());
	this->mLinesFiltered.resize(0);

	if (!filter.empty())
	{
		// TODO: Can this be run in parallel to avoid lock up on the main thread?
		for (const auto& line : this->mLines)
		{
			if (this->matchesFilter(line, filter))
			{
				this->mLinesFiltered.push_back(line);
			}
		}
	}

	this->mCurrentFilter = filter;
}

bool Log::matchesFilter(std::string const &item, std::string const&filter) const
{
	return item.find(filter) != std::string::npos;
}

void Log::renderView()
{
	this->renderMenuBar();
	this->renderFilterBar();
	this->renderContent();
}

void Log::renderMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::MenuItem("Clear", "", false, true)) this->clear();
		if (ImGui::MenuItem("Copy", "", false, true)) ImGui::LogToClipboard(); // will log all text output of ImGui to os clipboard until ???
		if (ImGui::BeginMenu("Options"))
		{
			if (ImGui::MenuItem("Autoscroll", "", this->mbAutoScroll, true)) this->mbAutoScroll = !this->mbAutoScroll;
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void Log::renderFilterBar()
{
	if (ImGui::InputText("Filter", this->mRawFilter.data(), this->mRawFilter.size()))
	{
		this->updateFilter();
	}
}

void Log::renderContent()
{
	ImGui::BeginChild("scroll-area", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	this->renderLines(this->mCurrentFilter.empty() ? this->mLines : this->mLinesFiltered);
	ImGui::PopStyleVar();

	if (this->mbAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);

	ImGui::EndChild();
}

void Log::renderLines(std::vector<std::string> const &lines)
{
	ImGuiListClipper clipper;
	// initialize the clipper with the number of entries
	clipper.Begin((i32)lines.size());
	while (clipper.Step())
	{
		for (i32 idxLine = clipper.DisplayStart; idxLine < clipper.DisplayEnd; ++idxLine)
		{
			ImGui::TextUnformatted(lines[idxLine].c_str());
		}
	}
	clipper.End();
}
