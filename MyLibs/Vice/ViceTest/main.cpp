#include "GLBase/sdlgl.hpp"
#include "Render/rendercontext.hpp"
#include "Render/rendergl.h"
#include "Render/utils_screen_space.h"
#include <fstream>

#include "Vice/component.h"
#include "Utils/on_scope_exit.h"
#include "Vice/vice.h"
#include "Vice/renderer.h"
#include "Utils/time_type.h"

using namespace glbase;
using namespace render;
using namespace scene;
using namespace std;
using namespace math;

void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam);

void python_script_test();
void component_test();
void gl_test();

int main(int argc, char* argv[])
{
	std::string mode;

	if(argc >= 2)
		mode = std::string(argv[1]);

	if(mode == "script")
	{
		python_script_test();
	}
	else if(mode == "component")
	{
		component_test();
	}
	else
	{
		gl_test();
	}
	return 0;
}

void interpreter() 
{
	using namespace vice;
	using namespace boost::python;

	try
	{
		object main_module = import("__main__");
		object main_namespace = main_module.attr("__dict__");

		std::cout << "\n> ";

		object str_fn = eval("str", main_namespace);

		for(;;)
		{
			std::string cmd;
			std::getline(std::cin, cmd);
			if(cmd == "quit")
				break;

			object ret;
			try
			{
				ret = exec_statement(cmd.c_str(), main_namespace);
			}
			catch(boost::python::error_already_set)
			{
				std::cerr << "\nPython error: ";
				PyErr_Print();
			}

			if(!ret.is_none())
				std::cout << "\n" << extract<const char*>(str_fn(ret))() << "\n";

			std::cout << "\n> ";
		}
	}
	catch(boost::python::error_already_set)
	{
		std::cerr << "\nPython error: ";
		PyErr_Print();
	}
}

void python_script_test()
{
	using namespace vice;
	using namespace boost::python;

	Py_Initialize();
	init_vice();

	interpreter();
}

void component_test()
{
	using namespace vice;
	using namespace boost::python;

	Py_Initialize();
	init_vice();

	try
	{
		ComponentLibrary::load_component("../Data/Vice/test.xml");
		ComponentInstance::ptr comp = ComponentLibrary::instance_component("../Data/Vice/test.xml", "inst");
		comp->mouse_move(1.0f, 2.0f);
		comp->mouse_down(3.0f, 4.0f, 1);
		comp->mouse_up(6.0f, 7.0f, 2);
	}
	catch(boost::python::error_already_set)
	{
		std::cerr << "\nPython error: ";
		PyErr_Print();
	}

	interpreter();
}

void gl_test()
{
	using namespace vice;
	using namespace boost::python;

#if defined(_DEBUG)
	static const unsigned int SCREEN_WIDTH = 1280;
	static const unsigned int SCREEN_HEIGHT = 720;
#else
	static const unsigned int SCREEN_WIDTH = 1920;
	static const unsigned int SCREEN_HEIGHT = 1080;
#endif	

	cout << "Initializing SDL ...\n";
	SDLGl::initSDL();
	cout << "Initializing OpenGL window ...\n";

	SDLGl::initOpenGL(SCREEN_WIDTH, SCREEN_HEIGHT, false);

	if(glewIsSupported("GL_ARB_debug_output"))
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_MARKER, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageCallback(gl_debug_callback, NULL);
	}
	//else if(glewIsSupported("GL_AMD_debug_output"))
	//	glDebugMessageCallbackAMD(gl_debug_callback_amd, NULL);

	cout << "Loading font ...\n";
	ilInit();
	iluInit();

	auto scopedRender = RenderGL::static_init();

	// init vice
	cout << "Init Vice ...\n";

	ComponentInstance::ptr comp;
	try
	{
		Py_Initialize();
		init_vice();
		ComponentLibrary::load_component("../Data/Vice/test.xml");
		comp = ComponentLibrary::instance_component("../Data/Vice/test.xml", "inst");
	}
	catch(boost::python::error_already_set)
	{
		std::cerr << "\nPython error: ";
		PyErr_Print();
		return;
	}


	glbase::Texture::ptr uiTexture = std::make_shared<glbase::Texture>("ui texture");
	uiTexture->create2D(GL_TEXTURE_RECTANGLE, SDLGl::width(), SDLGl::height(), 4, GL_RGBA, GL_UNSIGNED_BYTE, nullptr, GL_RGBA8);
	glbase::Texture::ptr background = std::make_shared<glbase::Texture>("ui background");
	background->create2D(GL_TEXTURE_RECTANGLE, SDLGl::width(), SDLGl::height(), 4, GL_RGBA, GL_UNSIGNED_BYTE, nullptr, GL_RGBA8);

	// init scene
	SceneContext::ptr sceneContext(new SceneContext());

	// --------------------------------- SCENE STAGE
	FramebufferObject::ptr fbo = std::make_shared<FramebufferObject>();
	fbo->AttachTexture(background->handle(), GL_COLOR_ATTACHMENT0_EXT);
	render::GeometryRenderStage::ptr backgroundRender = render::utils::create_2D_render_stage(SDLGl::width(), SDLGl::height(), "scene", fbo);
	backgroundRender->set_flag(RenderStageFlags::ClearColour | RenderStageFlags::ClearDepth);
	backgroundRender->set_clear_screen_colour(math::Vector4f(0.0f, 0.5f, 0.5f, 1.0f));

	// --------------------------------- UI STAGE
	render::UIRenderStage::ptr uiStage = std::make_shared<render::UIRenderStage>("ui render");
	comp->set_texture(uiTexture);
	comp->set_background(background);
	uiStage->add_context(comp);
	uiStage->add_dependancy(backgroundRender);

	// --------------------------------- UI OVERLAY STAGE
	effect::Effect::ptr renderUIEffect(new effect::Effect());
	renderUIEffect->load("../Data/Shaders/ui_overlay.xml");
	scene::Material::ptr renderUIMaterial(new scene::Material());
	renderUIMaterial->set_effect(renderUIEffect);
	renderUIMaterial->set_parameter("UITexture", uiTexture);
	render::GeometryRenderStage::ptr renderUIOverlayStage = render::utils::create_fsq_render_stage(SDLGl::width(), SDLGl::height(), renderUIMaterial, "ui overlay");
	renderUIOverlayStage->add_dependancy(uiStage);

	sceneContext->addStage(renderUIOverlayStage);

//#define API_TRACE
#if defined(API_TRACE)
	RenderGL::render(*sceneContext);
	SDLGl::swapBuffers();
	RenderGL::render(*sceneContext);
	SDLGl::swapBuffers();
#else
	cout << "Entering main loop\n";
	bool quit = false;
	int lastFPSFrame = 0;
	for(int f = 1; !quit; ++f)
	{
		SDL_Event event;
		SDL_PumpEvents();
		while(SDL_PollEvent(&event))
		{
			switch(event.type) {
			case SDL_KEYDOWN:
				switch(event.key.keysym.scancode)
				{
				case SDL_SCANCODE_ESCAPE: // esc
					quit = true; 
					break;
				};
				break;
			case SDL_QUIT: 
				quit = true;	
				break;
			};
		}
		comp->update(0);
		RenderGL::render(*sceneContext);
		SDLGl::swapBuffers();
	}
#endif
	std::cout << "Shutting down rendering...\n";
	SDLGl::shutDownSDL();

	std::cout << "Finished\n";
}

void FormatDebugOutputARB(char outStr[], size_t outStrSize, GLenum source, GLenum type,
						  GLuint id, GLenum severity, const char *msg)
{
	char sourceStr[32];
	const char *sourceFmt = "UNDEFINED(0x%04X)";
	switch(source)

	{
	case GL_DEBUG_SOURCE_API_ARB:             sourceFmt = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:   sourceFmt = "WINDOW_SYSTEM"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: sourceFmt = "SHADER_COMPILER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:     sourceFmt = "THIRD_PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION_ARB:     sourceFmt = "APPLICATION"; break;
	case GL_DEBUG_SOURCE_OTHER_ARB:           sourceFmt = "OTHER"; break;
	}

	_snprintf_s(sourceStr, 32, _TRUNCATE, sourceFmt, source);

	char typeStr[32];
	const char *typeFmt = "UNDEFINED(0x%04X)";
	switch(type)
	{
	case GL_DEBUG_TYPE_ERROR_ARB:               typeFmt = "ERROR"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: typeFmt = "DEPRECATED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  typeFmt = "UNDEFINED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB:         typeFmt = "PORTABILITY"; break;
	case GL_DEBUG_TYPE_PERFORMANCE_ARB:         typeFmt = "PERFORMANCE"; break;
	case GL_DEBUG_TYPE_OTHER_ARB:               typeFmt = "OTHER"; break;
	}
	_snprintf_s(typeStr, 32, _TRUNCATE, typeFmt, type);

	char severityStr[32];
	const char *severityFmt = "UNDEFINED";
	switch(severity)
	{
	case GL_DEBUG_SEVERITY_HIGH_ARB:   severityFmt = "HIGH";   break;
	case GL_DEBUG_SEVERITY_MEDIUM_ARB: severityFmt = "MEDIUM"; break;
	case GL_DEBUG_SEVERITY_LOW_ARB:    severityFmt = "LOW"; break;
	}

	_snprintf_s(severityStr, 32, _TRUNCATE, severityFmt, severity);

	_snprintf_s(outStr, outStrSize, _TRUNCATE, "OpenGL: %s [source=%s type=%s severity=%s id=%d]",
		msg, sourceStr, typeStr, severityStr, id);
}

std::ofstream glLog("GLLog.txt");

void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam)
{
	char finalMessage[1024];
	FormatDebugOutputARB(finalMessage, 1024, source, type, id, severity, message);
	std::cout << "OpenGL error: " << finalMessage << std::endl;
	glLog << finalMessage << std::endl;
}
