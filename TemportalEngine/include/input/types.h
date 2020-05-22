#pragma once

#include "TemportalEnginePCH.hpp"

#include <functional>
#include <map>

#include "Event.hpp"

NS_INPUT

typedef std::function<void(Event const &evt)> Listener;
typedef std::multimap<EInputType, Listener> ListenerMap;
typedef ListenerMap::iterator ListenerHandle;

NS_END