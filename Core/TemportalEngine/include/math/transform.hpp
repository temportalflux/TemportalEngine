#pragma once

#include "math/Matrix.hpp"

NS_MATH

/**
 * Returns a matrix which represents an object at `eyePos` looking at the position `center` where the `up` vector indicates the vertical direction.
 */
Matrix4x4 lookAt(Vector3 const &eyePos, Vector3 const &center, Vector3 const &up);

NS_END
