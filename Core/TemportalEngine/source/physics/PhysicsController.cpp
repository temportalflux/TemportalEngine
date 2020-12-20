#include "physics/PhysicsController.hpp"

#include "physics/PhysicsScene.hpp"
#include "physics/PhysX.hpp"
#include "utility/Casting.hpp"

using namespace physics;

Controller::Controller()
{
	this->mDescription.up = math::V3_UP;
}

Controller::DescTypeExt::DescTypeExt()
{
}

Controller::DescTypeExt::DescTypeExt(Controller::DescTypeExt const& other)
{
	memcpy_s(this, sizeof(Controller::DescTypeExt), &other, sizeof(Controller::DescTypeExt));
}

Controller::~Controller()
{
	release();
}

void Controller::release()
{
	if (this->mpInternal != nullptr)
	{
		as<physx::PxController>(this->mpInternal)->release();
		this->mpInternal = nullptr;
	}
}

Controller& Controller::setScene(std::weak_ptr<physics::Scene> pScene)
{
	this->mpScene = pScene;
	return *this;
}

Controller& Controller::setAsBox(math::Vector3 const& halfExtents)
{
	this->mDescription.type = physx::PxControllerShapeType::eBOX;
	this->mDescription.typed.box.halfExtents = halfExtents;
	return *this;
}

Controller& Controller::setMaterial(physics::Material *pMaterial)
{
	this->mDescription.pMaterial = pMaterial;
	return *this;
}

Controller& Controller::setCenterPosition(math::Vector<f64, 3> const& position)
{
	this->mDescription.position = position;
	return *this;
}

Controller& Controller::create()
{
	this->mpScene.lock()->createController(*this);
	return *this;
}
