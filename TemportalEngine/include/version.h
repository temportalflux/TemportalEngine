#pragma once

// Creates a unique 32-bit integer version for a unique semantic version
// NOTE: based on vulkan's VK_MAKE_VERSION
#define TE_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define TE_GET_MAJOR_VERSION(version) ((ui32)(version) >> 22)
#define TE_GET_MINOR_VERSION(version) (((ui32)(version) >> 12) & 0x3ff)
#define TE_GET_PATCH_VERSION(version) ((ui32)(version) & 0xfff)
