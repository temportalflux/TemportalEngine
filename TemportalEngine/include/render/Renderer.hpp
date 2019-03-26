#ifndef TE_RENDERER_HPP
#define TE_RENDERER_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Engine ---------------------------------------------------------------------
#include "types/integer.h"
#include "input/Event.hpp"

struct VkInstance;

NS_RENDER

class TEMPORTALENGINE_API Renderer
{

private:

public:
	Renderer();
	~Renderer();

};

NS_END

#endif
