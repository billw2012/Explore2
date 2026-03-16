
#include <vector>
#include "sdlgl.hpp"
#include <iostream>
#include <string>

namespace glbase {;

bool _sdlInitialized = false;
bool _openGLInitialized = false;
SDL_Window *_window = NULL;
SDL_GLContext _glContext;
unsigned int _viewWidth = 0;
unsigned int _viewHeight = 0;
std::vector<std::string> _extensions, _glslVersions;
std::string _vendor;
std::string _versionGL;
std::string _versionGLSL;

bool gCheckOpenGLErrors = true;
void set_check_opengl_errors(bool enabled)
{
	gCheckOpenGLErrors = enabled;
}

void check_opengl_errors()
{
	if (gCheckOpenGLErrors)
		glGetError();
		//assert(glGetError() == GL_NO_ERROR);
}

// struct SDLInitScope
// {
// 	~SDLInitScope()
// 	{
// 		SDLGl::shutDownSDL();
// 	}
// };
// 
// SDLInitScope _initScope;
// 
void checkSDLError(int line = -1)
{
#ifndef NDEBUG
	const char *error = SDL_GetError();
	if (*error != '\0')
	{
		printf("SDL Error: %s\n", error);
		if (line != -1)
			printf(" + line: %i\n", line);
		SDL_ClearError();
	}
#endif
}

void SDLGl::initSDL() /*throw (std::runtime_error)*/
{
	if(_sdlInitialized)
		return;
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		SDL_Quit();
		throw std::runtime_error("SDL library initialization failed!");
	}
	_sdlInitialized = true;
}

void SDLGl::shutDownSDL()
{
	if(_sdlInitialized)
		SDL_Quit();
	
	SDL_GL_DeleteContext(_glContext);
	SDL_DestroyWindow(_window);

	_sdlInitialized = false;
}

// void setVideoMode( unsigned int width, unsigned int height, unsigned int bpp, bool fullscreen )
// {
// 
// }

std::string null_safe_string(const GLubyte* str)
{
	if(str == NULL)
		return std::string("<NULL>");
	return std::string((const char*)str);
}


void init_open_gl_context(unsigned int width, unsigned int height)
{
	/* Request opengl 4.1 context.
	* SDL doesn't have the ability to choose which profile at this time of writing,
	* but it should default to the core profile */

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	int glFlags = SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG;
#if defined(_DEBUG)
	glFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, glFlags);

	_glContext = SDL_GL_CreateContext(_window);
	if (_glContext == 0)
	{
		std::string errMsg = "Cannot create GL context: ";
		errMsg += SDL_GetError();
		throw std::runtime_error(errMsg);
	}

	checkSDLError(__LINE__);

	SDL_GL_MakeCurrent(_window, _glContext);

	GLboolean doubleBuffered;
	glGetBooleanv(GL_DOUBLEBUFFER, &doubleBuffered);
	assert(doubleBuffered);

	std::cout << "Initializing OpenGL extensions ..." << std::endl;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		std::cout << glewGetErrorString(err) << std::endl;
		throw std::runtime_error((const char*)glewGetErrorString(err));
	}

	glViewport(0, 0, width, height);
	_viewWidth = width;
	_viewHeight = height;

	//_extensions = (const char*)glGetStringi(GL_EXTENSIONS);
	_vendor = null_safe_string(glGetString(GL_VENDOR));
	std::cout << "OpenGL vendor: " << _vendor << std::endl;
	_versionGL = null_safe_string(glGetString(GL_VERSION));
	std::cout << "OpenGL version: " << _versionGL << std::endl;
	_versionGLSL = null_safe_string(glGetString(GL_SHADING_LANGUAGE_VERSION));
	std::cout << "OpenGL shading language version: " << _versionGLSL << std::endl;

	{
		int extentionCount;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extentionCount);
		for (int idx = 0; idx < extentionCount; ++idx)
			_extensions.push_back(null_safe_string(glGetStringi(GL_EXTENSIONS, idx)));
	}
	{
		int glslVersionCount;
		glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &glslVersionCount);
		for (int idx = 0; idx < glslVersionCount; ++idx)
			_glslVersions.push_back(null_safe_string(glGetStringi(GL_SHADING_LANGUAGE_VERSION, idx)));
	}

	_openGLInitialized = true;
}

void SDLGl::init_opengl_from_hwnd(HWND window, unsigned int width, unsigned int height)
{
	if (!_sdlInitialized)
		throw std::exception("Trying to initialize OpenGL before SDL has been initialized!");

	_window = SDL_CreateWindowFrom(reinterpret_cast<const void*>(window));

	if (_window == NULL)
	{
		std::string errMsg = "Cannot create window: ";
		errMsg += SDL_GetError();
		throw std::runtime_error(errMsg);
	}
	checkSDLError(__LINE__);

	init_open_gl_context(width, height);

	_openGLInitialized = true;
}

void SDLGl::initOpenGL( unsigned int width, unsigned int height, bool fullscreen /*= false*/ ) /*throw (std::exception, std::runtime_error)*/
{
	if(!_sdlInitialized)
		throw std::exception("Trying to initialize OpenGL before SDL has been initialized!");

    /* Turn on double buffering with a 24bit Z buffer.
     * You may need to change this to 16 or 32 for your system */
	unsigned int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	if(fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;
	_window = SDL_CreateWindow("window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
	if (_window == NULL)
	{
		std::string errMsg = "Cannot create window: ";
		errMsg += SDL_GetError();
		throw std::runtime_error(errMsg);
	}
	checkSDLError(__LINE__);

	init_open_gl_context(width, height);

	_openGLInitialized = true;
}

bool SDLGl::sdlInitialized()
{
	return _sdlInitialized;
}

bool SDLGl::openGLInitialized()
{
	return _openGLInitialized;
}

void SDLGl::swapBuffers() /*throw (std::exception)*/
{
	if(!_openGLInitialized)
		throw std::exception("Trying to swap buffers before OpenGL has been initialized!");
	SDL_GL_SwapWindow(_window);
}

unsigned int SDLGl::width()
{
	return _viewWidth;
}

unsigned int SDLGl::height()
{
	return _viewHeight;
}

}