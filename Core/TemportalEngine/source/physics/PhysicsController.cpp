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
	this->scene()->createController(*this);
	return *this;
}

std::shared_ptr<physics::Scene> Controller::scene() const
{
	return this->mpScene.lock();
}

Controller& Controller::move(math::Vector3 const& displacement, f32 const& deltaTime)
{
	auto* pController = as<physx::PxController>(this->mpInternal);
	pController->move(
		physics::toPhysX(displacement), 0.0f, deltaTime,
		physx::PxControllerFilters(0)
	);
	return *this;
}

math::Vector<f64, 3> Controller::position() const
{
	auto* pController = as<physx::PxController>(this->mpInternal);
	return physics::fromPhysX(pController->getPosition());
}

f32 Controller::halfHeight() const
{
	switch (physx::PxControllerShapeType::Enum(this->mDescription.type))
	{
		case physx::PxControllerShapeType::Enum::eBOX:
			return this->mDescription.typed.box.halfExtents.y();
		case physx::PxControllerShapeType::Enum::eCAPSULE:
		case physx::PxControllerShapeType::Enum::eFORCE_DWORD:
		default: assert(false); return 0.0f;
	}
}

math::Vector<f64, 3> Controller::footPosition() const
{
	auto* pController = as<physx::PxController>(this->mpInternal);
	return physics::fromPhysX(pController->getFootPosition());
}
