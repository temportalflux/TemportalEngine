#pragma once

#ifdef TEMPORTALENGINE_EXPORTS
//#define TEMPORTALENGINE_API __declspec(dllexport)
#define TEMPORTALENGINE_API
#else
//#define TEMPORTALENGINE_API __declspec(dllimport)
#define TEMPORTALENGINE_API 
#endif
