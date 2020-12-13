#include "physics/PhysicsSystem.hpp"

#include "Engine.hpp"
#include "physics/PhysX.hpp"
#include "physics/PhysicsScene.hpp"
#include <extensions/PxDefaultCpuDispatcher.h>
#include <extensions/PxDefaultSimulationFilterShader.h>

using namespace physics;

static physx::PxDefaultAllocator gAllocator;
static ErrorCallback gErrorCallback;

#ifdef NDEBUG
bool shouldRecordMemoryAllocsByDefault = false;
#else
bool shouldRecordMemoryAllocsByDefault = true;
#endif

physics::System::System()
	: mVersion({ TE_PHYSX_VERSION })
	, mLogger(DeclareLog("Physics"))
	, mbRecordMemoryAllocations(shouldRecordMemoryAllocsByDefault)
	, mUnitLength(1.0f), mObjectSpeed(9.806f)
{
	gErrorCallback.init(&this->mLogger);
	this->mLogger.log(LOG_INFO, "Initializing PhysX %s", this->mVersion.toString().c_str());
	this->mpFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
}

physics::System::~System()
{
	this->uninit();
	
	this->foundation<physx::PxFoundation>()->release();
	this->mpFoundation = nullptr;

	gErrorCallback.invalidate();
}

physics::System& physics::System::setTolerances(f32 const& unitLength, f32 const& objectSpeed)
{
	this->mUnitLength = unitLength;
	this->mObjectSpeed = objectSpeed;
	return *this;
}

physics::System& physics::System::init(bool bUseDebugger)
{
	assert(this->mpFoundation);

	auto foundation = this->foundation<physx::PxFoundation>();
	physx::PxPvd* debugger = nullptr;

	if (bUseDebugger)
	{
		this->mLogger.log(LOG_INFO, "Initializing PhysX Visual Debugger");
		this->mpPhysxVisualDebugger = physx::PxCreatePvd(*foundation);
		debugger = this->debugger<physx::PxPvd>();
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate(
			"127.0.0.1", // ip-address of the debugger to connect to
			5425, // port
			10 // timeout (ms)
		);
		debugger->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
	}

	auto tolerance = physx::PxTolerancesScale();
	tolerance.length = this->mUnitLength;
	tolerance.speed = this->mObjectSpeed;
	
	this->mpPhysX = PxCreatePhysics(
		PX_PHYSICS_VERSION, *foundation, tolerance,
		this->mbRecordMemoryAllocations && debugger != nullptr, debugger
	);

	this->mpCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(/*thread count*/ 2);

	return *this;
}

void physics::System::uninit()
{
	this->dispatcher<physx::PxDefaultCpuDispatcher>()->release();
	this->mpCpuDispatcher = nullptr;

	if (this->mpPhysX)
	{
		this->pxphysx<physx::PxPhysics>()->release();
		this->mpPhysX = nullptr;
	}

	if (this->mpPhysxVisualDebugger)
	{
		auto pDebugger = this->debugger<physx::PxPvd>();
		auto transport = pDebugger->getTransport();
		pDebugger->release();
		transport->release();
		this->mpPhysxVisualDebugger = nullptr;
	}
}

void* physics::System::createScene(Scene *pScene)
{
	assert(this->mpPhysX && this->mpCpuDispatcher);
	auto pPhysX = this->pxphysx<physx::PxPhysics>();
	
	auto desc = physx::PxSceneDesc(pPhysX->getTolerancesScale());
	desc.gravity = toPhysX(pScene->gravity());
	desc.cpuDispatcher = this->dispatcher<physx::PxDefaultCpuDispatcher>();
	desc.filterShader = physx::PxDefaultSimulationFilterShader;
	assert(desc.isValid());
	
	return pPhysX->createScene(desc);
}
