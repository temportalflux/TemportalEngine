#include "physics/PhysicsShape.hpp"

#include "physics/PhysicsSystem.hpp"
#include "physics/PhysX.hpp"
#include "utility/Casting.hpp"

using namespace physics;

Shape::TypeData::TypeData()
{
	memset(this, 0, sizeof(TypeData));
}

Shape::Shape() : mType(EShapeType::eInvalid), mTypeData(), mpInternal(nullptr)
{
}

Shape::~Shape()
{
	if (this->mpInternal != nullptr)
	{
		assert(hasSystem());
		as<physx::PxShape>(this->mpInternal)->release();
		this->mpInternal = nullptr;
	}
}

Shape& Shape::setMaterial(physics::Material *pMaterial)
{
	this->mpMaterial = pMaterial;
	return *this;
}

Shape& Shape::setAsSphere(f32 const& radius)
{
	this->mType = EShapeType::eSphere;
	this->mTypeData.sphere.radius = radius;
	return *this;
}

Shape& Shape::setAsPlane(math::Vector3 const& normal, f32 const& distance)
{
	this->mType = EShapeType::ePlane;
	this->mTypeData.plane.normal = normal;
	this->mTypeData.plane.distance = distance;
	return *this;
}

Shape& Shape::setAsBox(math::Vector3 const& halfExtents)
{
	this->mType = EShapeType::eBox;
	this->mTypeData.box.halfExtents = halfExtents;
	return *this;
}

EShapeType const& Shape::type() const { return this->mType; }

void Shape::create()
{
	this->mpInternal = this->system()->createShape(this);
}
