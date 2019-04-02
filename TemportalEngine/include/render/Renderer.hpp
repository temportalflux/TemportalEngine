#ifndef TE_RENDERER_HPP
#define TE_RENDERER_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Engine ---------------------------------------------------------------------
#include "types/integer.h"
#include "input/Event.hpp"

NS_RENDER

class TEMPORTALENGINE_API Renderer
{

private:

public:
	Renderer();
	~Renderer();

	void initializeWindow();
	void render();

};

NS_END

#endif
