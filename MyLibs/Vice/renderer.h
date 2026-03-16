#pragma once

#include "component.h"
#include "GLBase\texture.hpp"

namespace vice {;

struct Renderer
{
	static void render(const ComponentInstance& component);
};

}