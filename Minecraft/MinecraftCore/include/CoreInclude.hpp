#pragma once

#include "TemportalEnginePCH.hpp"
#include "Delegate.hpp"

#define CHUNK_SIDE_LENGTH 16
#define CHUNK_HALF_LENGTH (CHUNK_SIDE_LENGTH / 2)
#define FOR_CHUNK_SIZE(TYPE, IDX) for (TYPE IDX = 0; IDX < CHUNK_SIDE_LENGTH; ++IDX)
#define FOR_CHUNK_SIZE_PLUS(TYPE, IDX, START, END) for (TYPE IDX = START; IDX < CHUNK_SIDE_LENGTH + END; ++IDX)

#define NS_GAME namespace game {
#define NS_WORLD namespace world {
#define NS_RESOURCE namespace resource {
