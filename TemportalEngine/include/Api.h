#pragma once

#ifdef TEMPORTALENGINE_EXPORTS  
#define TEMPORTALENGINE_API __declspec(dllexport)   
#else
#define TEMPORTALENGINE_API __declspec(dllimport)   
#endif
