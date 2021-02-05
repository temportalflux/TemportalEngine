#include "physics/PhysicsScene.hpp"

#include "physics/PhysicsMaterial.hpp"
#include "physics/PhysicsRigidBody.hpp"
#include "physics/PhysicsSystem.hpp"
#include "physics/PhysX.hpp"
#include "utility/Casting.hpp"

using namespace physics;

Scene::Scene()
	: mGravity(math::V3_UP * -9.806f)
	, mpInternal(nullptr)
{
}

Scene::~Scene()
{
	if (this->mpControllerManager != nullptr)
	{
		as<physx::PxControllerManager>(this->mpControllerManager)->release();
		this->mpControllerManager = nullptr;
	}
	if (this->mpInternal != nullptr)
	{
		as<physx::PxScene>(this->mpInternal)->release();
		this->mpInternal = nullptr;
	}
}

void Scene::create()
{
	this->mpInternal = this->system()->createScene(this);
	this->mpControllerManager = PxCreateControllerManager(*as<physx::PxScene>(this->mpInternal));
}

Scene& Scene::setGravity(math::Vector3 const& gravity)
{
	this->mGravity = gravity;
	return *this;
}

math::Vector3 const& Scene::gravity() const { return this->mGravity; }

Scene& Scene::addActor(RigidBody *pBody)
{
	auto pScene = as<physx::PxScene>(this->mpInternal);
	pScene->addActor(*extract<physx::PxRigidBody>(pBody));
	return *this;
}

void populateGeneralControllerDesc(physx::PxControllerDesc &desc, physics::Controller::Description const& wrap)
{
	desc.position = physx::PxExtendedVec3(wrap.position.x(), wrap.position.y(), wrap.position.z());
	desc.upDirection = physics::toPhysX(wrap.up);
	desc.material = as<physx::PxMaterial>(wrap.pMaterial->get());
}

void Scene::createController(Controller &controller)
{
	physx::PxControllerManager* manager = as<physx::PxControllerManager>(this->mpControllerManager);
	switch (physx::PxControllerShapeType::Enum(controller.mDescription.type))
	{
		case physx::PxControllerShapeType::Enum::eBOX:
		{
			physx::PxBoxControllerDesc desc = {};
			desc.halfSideExtent = controller.mDescription.typed.box.halfExtents.x();
			desc.halfHeight = controller.mDescription.typed.box.halfExtents.y();
			desc.halfForwardExtent = controller.mDescription.typed.box.halfExtents.z();
			populateGeneralControllerDesc(desc, controller.mDescription);
			controller.mpInternal = manager->createController(desc);
			break;
		}
		case physx::PxControllerShapeType::Enum::eCAPSULE: assert(false); break;
		default: break;
	}
}

ui32 Scene::getControllerCount() const
{
	return as<physx::PxControllerManager>(this->mpControllerManager)->getNbControllers();
}

void Scene::getController(ui32 const& index, Controller &out) const
{
	void* pController = as<physx::PxControllerManager>(this->mpControllerManager)->getController(index);
	out.mpInternal = pController;
}

void Scene::simulate(f32 const& deltaTime)
{
	assert(this->mpInternal);
	auto pScene = as<physx::PxScene>(this->mpInternal);
	pScene->simulate(deltaTime);
	pScene->fetchResults(true);
}
