#ifndef _RENDER_RENDERGL_HPP
#define _RENDER_RENDERGL_HPP

#include <stdexcept>
#include <set>
#include <map>
#include <functional>
#include <boost/iterator/transform_iterator.hpp>
#include "Math/frustum.hpp"
#include "Math/intersection.hpp"
#include "GLBase/sdlgl.hpp"
#include "GLBase/videomemorymanager.hpp"
#include "scenecontext.hpp"
#include "semantic_param_set.hpp"
#include "fontgl.h"

namespace render {;

struct RenderGL
{
	//RenderGL();
	static std::shared_ptr<void> static_init();
	static void static_release();
	//void setVMM(VideoMemoryManager::ptr vmm);
	// to render with multiple lights:
	// for each light determine the meshes it effects
	// render a shadow map using those objects as shadow casters
	// for each mesh render using active lights and shadow maps for the lights
	static void render(const SceneContext& sc);
	static void draw_text(int x, int y, const std::string& str, const FontGL& font);
};

}

#endif // _RENDER_RENDERGL_HPP