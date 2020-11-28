#pragma once

#include "TemportalEnginePCH.hpp"

#include "FixedSortedArray.hpp"
#include "ObjectPool.hpp"
#include "ecs/types.h"
#include "ecs/entity/EntityManager.hpp"
#include "ecs/component/ComponentManager.hpp"
#include "ecs/view/ECViewManager.hpp"
#include "logging/Logger.hpp"

NS_ECS

class Core
{

public:

	Core();
	~Core();

	Core& setLog(logging::Logger log);

	EntityManager& entities();
	component::Manager& components();
	view::Manager& views();

	template <typename... TArgs>
	void log(logging::ECategory category, logging::Message format, TArgs... args)
	{
		this->mLog.log(category, format, args...);
	}

private:
	logging::Logger mLog;
	EntityManager mEntityManager;
	component::Manager mComponentManager;
	view::Manager mViewManager;

};

NS_END
