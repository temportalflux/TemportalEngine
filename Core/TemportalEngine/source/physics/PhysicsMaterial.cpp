#include "physics/PhysicsMaterial.hpp"

#include "physics/PhysicsSystem.hpp"
#include "physics/PhysX.hpp"
#include "utility/Casting.hpp"

using namespace physics;

Material::Material()
	: mpInternal(nullptr)
	, mStaticFriction(0.5f)
	, mDynamicFriction(0.5f)
	, mRestitution(0.6f)
{
}

Material::Material(Material &&other)
{
	*this = std::move(other);
}

Material& Material::operator=(Material &&other)
{
	this->mStaticFriction = other.mStaticFriction;
	this->mDynamicFriction = other.mDynamicFriction;
	this->mRestitution = other.mRestitution;
	this->mpInternal = other.mpInternal;
	other.mpInternal = nullptr;
	return *this;
}

Material::~Material()
{
	if (this->mpInternal != nullptr)
	{
		assert(hasSystem());
		as<physx::PxMaterial>(this->mpInternal)->release();
		this->mpInternal = nullptr;
	}
}

void Material::create()
{
	this->mpInternal = this->system()->createMaterial(
		this->mStaticFriction, this->mDynamicFriction, this->mRestitution	
	);
}
