#pragma once

#include "TemportalEnginePCH.hpp"
#include "logging/Logger.hpp"
#include "utility/Version.hpp"

NS_PHYSICS
class Scene;

class System : public std::enable_shared_from_this<System>
{

public:
	System();
	~System();

	System& setTolerances(f32 const& unitLength, f32 const& objectSpeed);
	System& init(bool bUseDebugger);
	
	void* createScene(Scene *pScene);

private:
	Version const mVersion;
	logging::Logger mLogger;
	
	void* mpFoundation;
	void* mpPhysX;
	void* mpCpuDispatcher;

	void* mpPhysxVisualDebugger;
	bool mbRecordMemoryAllocations;
	
	f32 mUnitLength;
	f32 mObjectSpeed;

	template <typename T> T* foundation() const { return (T*)mpFoundation; }
	template <typename T> T* debugger() const { return (T*)mpPhysxVisualDebugger; }
	template <typename T> T* pxphysx() const { return (T*)mpPhysX; }
	template <typename T> T* dispatcher() const { return (T*)mpCpuDispatcher; }

	void uninit();

};

NS_END
