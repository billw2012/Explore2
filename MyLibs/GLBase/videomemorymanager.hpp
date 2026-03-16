#ifndef _RENDER_VIDEOMEMORYMANAGER_HPP
#define _RENDER_VIDEOMEMORYMANAGER_HPP

#include <stdexcept>
#include <list>
#include <map>
#include <hash_map>
//#define BOOST_BIND_ENABLE_STDCALL

#include <functional>
#include "sdlgl.hpp"
#include "vertexspec.hpp"
#include "vramobject.hpp"
#include "triangleset.hpp"
#include "texture.hpp"
//#include "memorymanager.hpp"

namespace glbase {;

struct VideoMemoryManager
{
	static bool load_buffers(const glbase::TriangleSet::ptr& triSet, const glbase::VertexSet::ptr& verts);
	static void render_current();
	static void unbind_buffers();
};

}

#endif // _RENDER_VIDEOMEMORYMANAGER_HPP