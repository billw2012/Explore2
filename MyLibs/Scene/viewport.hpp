
#if !defined(__SCENE_VIEWPORT_HPP__)
#define __SCENE_VIEWPORT_HPP__

#include "Math/rectangle.hpp"

namespace scene {;

struct Viewport : math::Rectanglei
{
	typedef std::shared_ptr< Viewport > ptr;

	Viewport() {}
	Viewport(int x, int y, int width, int height)
		: math::Rectanglei(x, y, x + width, y + height) {}

	math::Matrix4f matrix() const 
	{
		float wdiv2 = static_cast<float>(width()) * 0.5f;
		float hdiv2 = static_cast<float>(height()) * 0.5f;
		return math::Matrix4f (
			wdiv2,	0.0f,	0.0f,	wdiv2 + static_cast<float>(left),
			0.0f,	hdiv2,	0.0f,	hdiv2 + static_cast<float>(bottom),
			0.0f,	0.0f,	0.5f,	0.5f,
			0.0f,	0.0f,	0.0f,	1.0f
			);
	}
};

}

#endif // __SCENE_VIEWPORT_HPP__