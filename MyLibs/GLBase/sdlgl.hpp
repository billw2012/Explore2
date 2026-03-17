#ifndef _RENDER_SDLGL_HPP
#define _RENDER_SDLGL_HPP

#include <stdexcept>
#include <windows.h>
//#define GL_GLEXT_PROTOTYPES
#define NO_SDL_GLEXT
#define GLEW_NO_GLU
#include <gl/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <cassert>

#undef max
#undef min

//#pragma comment(lib, "glew32.lib")

namespace glbase {;

#if defined(_DEBUG)
void set_check_opengl_errors(bool enabled);
void check_opengl_errors();
#define CHECK_OPENGL_ERRORS glbase::check_opengl_errors();
#else
#define CHECK_OPENGL_ERRORS
#endif

struct SDLGl
{
	static void initSDL();
	static void shutDownSDL();
	static void init_opengl_from_hwnd(HWND window, unsigned int width, unsigned int height);
	static void initOpenGL(unsigned int width, unsigned int height, bool fullscreen = false);
	static bool sdlInitialized();
	static bool openGLInitialized();
	static void swapBuffers();
	static unsigned int width();
	static unsigned int height();
};

}

#endif // _RENDER_SDLGL_HPP