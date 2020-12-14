#include "physics/PhysicsScene.hpp"

#include "physics/PhysicsRigidBody.hpp"
#include "physics/PhysicsSystem.hpp"
#include "physics/PhysX.hpp"
#include "utility/Casting.hpp"

using namespace physics;

Scene::Scene()
	: mGravity(math::V3_UP * -9.806f)
	, mSimulationFrequency(1.0f / 60.0f), mTimeSinceLastSimulate(0.0f)
	, mpInternal(nullptr)
{
}

Scene::~Scene()
{
	if (this->mpInternal != nullptr)
	{
		as<physx::PxScene>(this->mpInternal)->release();
		this->mpInternal = nullptr;
	}
}

void Scene::create()
{
	this->mpInternal = this->system()->createScene(this);
}

Scene& Scene::setGravity(math::Vector3 const& gravity)
{
	this->mGravity = gravity;
	return *this;
}

math::Vector3 const& Scene::gravity() const { return this->mGravity; }

Scene& Scene::setSimulationStep(f32 const& frequency)
{
	this->mSimulationFrequency = frequency;
	return *this;
}

Scene& Scene::addActor(RigidBody *pBody)
{
	auto pScene = as<physx::PxScene>(this->mpInternal);
	pScene->addActor(*extract<physx::PxRigidBody>(pBody));
	return *this;
}

void Scene::simulate(f32 const& deltaTime)
{
	assert(this->mpInternal);
	this->mTimeSinceLastSimulate += deltaTime;
	if (this->mTimeSinceLastSimulate >= this->mSimulationFrequency)
	{
		this->mTimeSinceLastSimulate -= this->mSimulationFrequency;
		auto pScene = as<physx::PxScene>(this->mpInternal);
		pScene->simulate(this->mSimulationFrequency);
		pScene->fetchResults(true);
	}
}
