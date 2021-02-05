#pragma once

#include "TemportalEnginePCH.hpp"

NS_PHYSICS
class Scene;
class Material;

class Controller
{
	friend class Scene;

public:
	Controller();
	~Controller();
	void release();

	Controller& setScene(std::weak_ptr<physics::Scene> pScene);
	Controller& setAsBox(math::Vector3 const& halfExtents);
	Controller& setMaterial(physics::Material *pMaterial);
	Controller& setCenterPosition(math::Vector<f64, 3> const& position);
	Controller& create();
	
	std::shared_ptr<physics::Scene> scene() const;
	Controller& move(math::Vector3 const& displacement, f32 const& deltaTime);
	Controller& setFootPosition(math::Vector<f64, 3> const& position);

	math::Vector<f64, 3> position() const;
	f32 halfHeight() const;
	math::Vector<f64, 3> footPosition() const;

public:
	struct BoxType
	{
		math::Vector3 halfExtents;
	};
	union DescTypeExt
	{
		BoxType box;
		
		DescTypeExt();
		DescTypeExt(DescTypeExt const& other);
	};
	struct Description
	{
		// SPECIFIC
		DescTypeExt typed;

		// GENERAL
		ui8 type;
		math::Vector<f64, 3> position;
		math::Vector3 up;
		physics::Material *pMaterial;
	};

private:
	Description mDescription; // only present during creation

	std::weak_ptr<physics::Scene> mpScene;
	/*physx::PXController*/ void* mpInternal;

};

NS_END