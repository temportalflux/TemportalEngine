#include "physics/PhysX.hpp"

#include "logging/Logger.hpp"

using namespace physics;

ErrorCallback::ErrorCallback() : mpLogger(nullptr) {}
void ErrorCallback::init(logging::Logger *pLogger) { mpLogger = pLogger; }
void ErrorCallback::invalidate() { mpLogger = nullptr; }

void ErrorCallback::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
{
	assert(mpLogger != nullptr);
	logging::ECategory category = LOG_ERR;
	if (code == physx::PxErrorCode::eDEBUG_INFO) category = LOG_INFO;
	else if (code == physx::PxErrorCode::eDEBUG_WARNING || code == physx::PxErrorCode::ePERF_WARNING) category = LOG_WARN;
	this->mpLogger->log(category, "%s (%s line:%i)", message, file, line);
	switch (code)
	{
	case physx::PxErrorCode::eINVALID_PARAMETER:
	case physx::PxErrorCode::eINVALID_OPERATION:
	case physx::PxErrorCode::eOUT_OF_MEMORY:
	case physx::PxErrorCode::eINTERNAL_ERROR:
	case physx::PxErrorCode::eABORT:
		assert(false);
		break;
	default: break;
	}
}

physx::PxVec3 physics::toPhysX(math::Vector3 const& v)
{
	static uSize SIZE = sizeof(f32) * 3;
	physx::PxVec3 out;
	memcpy_s(&out, SIZE, v.data(), SIZE);
	return out;
}
