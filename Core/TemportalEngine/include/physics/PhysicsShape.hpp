#pragma once

#include "physics/PhysicsObject.hpp"

NS_PHYSICS
class Material;

enum class EShapeType : i8
{
	eSphere,
	ePlane,
	eCapsule,
	eBox,
	eConvexMesh,
	eTriangleMesh,
	eHeightField,
	eCount,
	eInvalid = -1,
};

class Shape : public Object
{
	friend class System;

public:
	Shape();
	~Shape();

	Shape& setMaterial(physics::Material *pMaterial);
	Shape& setAsSphere(f32 const& radius);
	Shape& setAsPlane(math::Vector3 const& normal, f32 const& distance);
	Shape& setAsBox(math::Vector3 const& halfExtents);

	EShapeType const& type() const;
	void create() override;
	bool isValid() const { return this->mpInternal != nullptr; }
	void* get() const { return this->mpInternal; }

private:
	physics::Material *mpMaterial;

	union TypeData
	{

		struct
		{
			f32 nil[3];
			f32 radius;
		} sphere;

		struct
		{
			math::Vector3 normal;
			f32 distance;
		} plane;

		struct
		{
			math::Vector3 halfExtents;
			f32 nil[1];
		} box;

		TypeData();
	};
	EShapeType mType;
	TypeData mTypeData;

	/*PxShape*/ void* mpInternal;

};

NS_END
