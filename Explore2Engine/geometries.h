
#ifndef geometries_h__
#define geometries_h__

#include "Math/vector3.hpp"
#include <functional>

namespace explore2 {;

struct Geometries 
{
	static void create_sphere(int gridSize, float radius, 
		std::function<void (const math::Vector3f&)> emitVertex, 
		std::function<void (int)> emitIndex,
		std::function<void (int)> emitFace );
};

}

#endif // geometries_h__