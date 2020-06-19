#pragma once

#include "TemportalEnginePCH.hpp"

NS_MATH

constexpr f64 pi() { return 3.1415926535897932384626433832795f; }
constexpr f64 pi2() { return pi() / 2.0f; }
constexpr f64 pi4() { return pi() / 4.0f; }
constexpr f64 epsilon() { return 1.19e-07; }

constexpr f32 toDegrees(f32 const radians) { return radians * 57.295779513082320876798154814105f; }
constexpr f32 toRadians(f32 const degrees) { return degrees * 0.01745329251994329576923690768489f; }

template <typename TNum>
constexpr TNum max(TNum const a, TNum const b) { return a > b ? a : b; }
template <typename TNum>
constexpr TNum min(TNum const a, TNum const b) { return a < b ? a : b; }

NS_END
