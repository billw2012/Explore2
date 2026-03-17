// Explore2.cpp : Defines the entry point for the console application.
//

#include "RemoteDebugInterface.h"

#include "GLBase/sdlgl.hpp"
#include "Scene/transform.hpp"
#include "Scene/group.hpp"
#include "Scene/drawable.hpp"
#include "Scene/camera.hpp"
#include "Render/rendercontext.hpp"
#include "Render/rendergl.h"
#include "Math/transformation.hpp"
#include "Render/utils_screen_space.h"

#include "Explore2Engine/solarsystem.h"
#include "Explore2Engine/gbuffer.h"

#include "Utils/Profiler.h"

#include "Explore2Engine/player.h"

#include <fstream>

//#define DISABLE_PLANET
#define RENDER_DEBUG_ENABLED

#if defined(RENDER_DEBUG_ENABLED)

#define DEBUG_RENDER_OFF			0
#define DEBUG_RENDER_ALBEDO			1
#define DEBUG_RENDER_PLANET_INDEX	2
#define DEBUG_RENDER_NORMAL			3
#define DEBUG_RENDER_POSITION		4
#define DEBUG_RENDER_CLIPZ			5
#define DEBUG_RENDER_DEPTH			6
#define DEBUG_RENDER_HEIGHT			7
#define DEBUG_RENDER_ALL			8
#define DEBUG_RENDER_PBUFFER0		9
#define DEBUG_RENDER_PBUFFER1		10
#define DEBUG_RENDER_MODE_COUNT		11

std::string get_debug_render_mode_name(int mode)
{
	switch(mode)
	{
	case DEBUG_RENDER_OFF			: return "off";
	case DEBUG_RENDER_ALBEDO		: return "albedo";
	case DEBUG_RENDER_PLANET_INDEX	: return "planet_index";
	case DEBUG_RENDER_NORMAL		: return "normal";
	case DEBUG_RENDER_POSITION		: return "position";
	case DEBUG_RENDER_CLIPZ			: return "clipz";	
	case DEBUG_RENDER_DEPTH			: return "depth";	
	case DEBUG_RENDER_HEIGHT		: return "height";	
	case DEBUG_RENDER_ALL			: return "all";
	case DEBUG_RENDER_PBUFFER0		: return "pbuffer0";
	case DEBUG_RENDER_PBUFFER1		: return "pbuffer1";
	};
	return "unknown";
}

#endif

using namespace glbase;
using namespace render;
using namespace scene;
using namespace std;
using namespace math;
using namespace explore2;
using namespace remote;

void stub_string_event_callback(std::string&){}
void stub_fps_event_callback(remote_utils::FrameTimeEvent&){}
void stub_chunk_stats_event_callback(remote_utils::ChunkLODStatsEvent&){}
void stub_texture_list_event_callback(remote_utils::TextureList&){}
void stub_texture_pixels_event_callback(remote_utils::TexturePixels&){}
void texture_request_event_callback(remote_utils::TextureRequest& request)
{
	if(request.index == std::numeric_limits<unsigned int>::max())
	{
		int texCount = TextureManager::get_count();
		remote_utils::TextureList texList;
		for(int idx = 0; idx < texCount; ++idx)
		{
			const glbase::Texture* tex = TextureManager::get(idx);
			texList.add_texture(idx, tex->handle(), tex->get_name(), tex->type(), tex->width(), tex->height(), tex->depth(), tex->internal_format());
		}
		RemoteDebug::get_instance()->send_event(texList);
	}
	else if(request.index < (unsigned int)TextureManager::get_count())
	{
		glbase::Texture::SimpleTextureData data;
		const glbase::Texture* tex = TextureManager::get(request.index);
		tex->get_simple(data, 0);
		remote_utils::TexturePixels sendData;
		sendData.width = data.width;
		sendData.height = data.height;
		sendData.depth = data.depth;
		sendData.pixels = std::move(data.pixels);
		RemoteDebug::get_instance()->send_event(sendData);
	}
}

void command_event_callback(remote_utils::Command& command)
{
	if(command.commandStr == "Reload Effects")
		effect::Effect::reload_all();
}

void chunk_lod_stats_visitor(remote_utils::ChunkLODStatsEvent::StatesVector& states, 
							 size_t& oneWeightCount,
							 size_t& zeroWeightCount,
							 ChunkLOD* chunk);
void update_atmosphere_material(scene::Material::ptr mat, 
								scene::transform::Camera::ptr camera, 
								scene::transform::Transform::ptr planetTransform);

void update_material_planet_light_dir(scene::Material::ptr mat, 
									  scene::transform::Light::ptr light,
									  scene::transform::Transform::ptr planetTransform);
void add_material_update(std::function< void() > fn);
void update_materials();

void FormatDebugOutputARB(char outStr[], size_t outStrSize, GLenum source, GLenum type,
						  GLuint id, GLenum severity, const char *msg);
scene::Geometry::ptr create_backplane(const scene::transform::Camera::ptr& camera);
void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam);
void APIENTRY gl_debug_callback_amd(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam);



/*
Optimizations:
in the chunk and planet systems:
- add more abort points to stop un-used data being calculated.
in the renderer:
- lazily evaluate and store matrix combinations (inverted model view etc.)
- only update shader parameters when they have changed
- only switch shader when it changes

To do:
DONE - Make sure calculation of parent verts of root chunks is done so that
it matches the triangulation of the plane sphere. 
DONE - Correct the error calculations on the chunks so that chunk children are not 
getting hidden when they should not be (stop chunks suddenly flicking to a lower
lod), maybe something to do with their bounding boxes not being correct? Once
data has been created use the aabb of the MeshTransform of the chunk. Once 
children have been created expand the aabb by their aabbs. When a chunks aabb
changes, update its ancestors aabbs. .
DONE - Move all matrix calculation to double prec. .
DONE - Wrap UVs use on source textures when creating chunk texture, so that they are 
not becoming huge and causing floating point errors to creep in. (not certain 
this is happening).
DONE - Split rendering into near and far sets, render each with its own depth buffer.
DONE, by switching to double prec. - Increase height precision by creating two sets of heights for each lod chunk,
using less octaves for each, then combining the two together AFTER localising
them to the chunk root.
- Add water, including under water shader?
- Add atmospheric scattering and atmosphere - move to deferred shading? Rendering split into
near and far makes this more difficult... Would also need tags on every pixel to 
say whether it is to have atmospheric scattering applied.
- Improve texturing of ground.
- Add trees/grass/plants/buildings/rock outcrops, etc (clutter). All should use
the same system.

- Share tris between all chunks, for water and land. 
- Switch near/far determination back to a flag, so that water and land do not
end up in different sets.
- Work out how to fade in and out water... 
*/

/*

Deferred shading:

Stage 1: render G buffers:
G buffers need to contain albedo, depth, specularity, normal:
buffers:
colour: 24 bit normal
depth: 24 bit depth
aux0: 24 bit diffuse
aux1: 24 bit specular + 8 bit shininess

Atmospheric scattering:

Reflective surfaces in the atmosphere are lit by light that has already been scattered. 
So pixels in the atmosphere first need to have their incoming light scattered, then need 
to have their reflected light scattered. Both stages require in and out scattering.
The first stage is effectively a lighting stage so could be done during lighting instead 
of during post processing.

For each planet->sun pair generate a texture for the inner integral:
one axis is height, one axis is angle to sun.
The inner integral represents the light scattered in and out on the way to the point.
This texture can be used to "light" the scene using the sun, and to quickly calculate
the inner integral of the atmospheric scattering pass.

Render pipeline now consists of:

1: render geometry into G buffer
2: render lights (including suns) into hdr A buffer 1, using G buffer as source
3: render atmospheric scattering into hdr A buffer 2, using A buffer 1 and G buffer as sources
4: calculate auto-exposure into hdr A buffer 3, using A buffer 2
5: expose and split the hdr A buffer 2 into saturated (hdr A buffer 3) and everything else (4)
6: bloom hdr A buffer 3 into hdr A buffer 4 horiz and back to 3 vert or something else depending on the bloom method
7: sum the bloom buffer (3) with the everything else buffer (4) into the front left buffer for display

1:
done
2: 
Each light render consists of:
a) determine set of objects visible to the light, if it is a directional light (i.e. a sun) then this is everything.
b) if it is a directional light create cascaded shadow maps, 
   if it is a point light create shadow cube map,
   if it is a spot light create a standard 2D shadow map.
   All shadow maps of the same type should share textures to save memory.
c) determine screen space rectangle that encloses entire set of lit objects, or full screen whichever is smaller
d) render screen space rectangle using appropriate shader for the light type.
3:
a) determine screen space rectangle that encloses entire atmosphere (not just planet), or full screen whichever is smaller.
b) render screen space rectangle using atmosphere shader.
4:

Lighting:
lights are of type directional, point or spot.
different types of light have different methods for generating shadows, and rendering of the light rectangle.



*/

using namespace scene::transform;


int main(int argc, char* argv[])
{
	RemoteDebug::create_instance(RemoteDebug::Game_t());

	remote_utils::register_event<std::string>(stub_string_event_callback);
	remote_utils::register_event<remote_utils::FrameTimeEvent>(stub_fps_event_callback);
	remote_utils::register_event<remote_utils::ChunkLODStatsEvent>(stub_chunk_stats_event_callback);
	remote_utils::register_event<remote_utils::TextureList>(stub_texture_list_event_callback);
	remote_utils::register_event<remote_utils::TexturePixels>(stub_texture_pixels_event_callback);
	remote_utils::register_event<remote_utils::TextureRequest>(texture_request_event_callback);
	remote_utils::register_event<remote_utils::Command>(command_event_callback);

	opencl::init();	
	HPCNoise::init();

	int abortFrames = -1;
	std::set<std::string> args;
	for(int idx = 1; idx < argc; ++idx)
		args.insert(argv[idx]);

#if defined(_DEBUG)
	if(args.find(std::string("no_assert")) != args.end())
		glbase::set_check_opengl_errors(false);
#endif

	FontGL font;
	cout << "Initializing SDL ..." << endl;
	SDLGl::initSDL();
	cout << "Initializing OpenGL window ..." << endl;
#if defined(_DEBUG)
	SDLGl::initOpenGL(1280, 720, false);
#else
	SDLGl::initOpenGL(1920, 1080, false);
#endif

 	if(glewIsSupported("GL_ARB_debug_output"))
	{
		//glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
 		glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(gl_debug_callback), NULL);
	}
	//else if(glewIsSupported("GL_AMD_debug_output"))
	//	glDebugMessageCallbackAMD(gl_debug_callback_amd, NULL);

	cout << "Loading font ..." << endl;
	ilInit();
	iluInit();
	font.load("../Data/Font.tga", "../Data/Shaders/font.xml");

	//cout << "Initializing CEFUI ..." << endl;
	//std::shared_ptr<void> scopedCEFUI = CEFUI::static_init("Explore2CEF.exe", "../Data/CEF", "../Data/CEF/locales");

	static const double NEAR_PLANE = 0.001 * WORLD_SCALE;
	static const double FAR_PLANE = 500000000 * WORLD_SCALE;
	effect::ShaderManager::set_define("NEAR_PLANE", effect::ShaderManager::define_variant_type((float)NEAR_PLANE));
	effect::ShaderManager::set_define("FAR_PLANE", effect::ShaderManager::define_variant_type((float)FAR_PLANE));

	auto scopedSolarSystem = SolarSystem::static_init();
	auto scopedRender = RenderGL::static_init();

	SceneContext::ptr sceneContext(new SceneContext());
	transform::Camera::ptr camera(new transform::Camera());
	scene::Viewport::ptr viewport(new Viewport(0, 0, SDLGl::width(), SDLGl::height()));
	camera->set_viewport(viewport);
	camera->set_near_plane(NEAR_PLANE);
	camera->set_far_plane(FAR_PLANE);

	GBuffer::ptr gbuffer(new GBuffer());
	gbuffer->init(SDLGl::width(), SDLGl::height());

	// --------------------------------- BACKGROUND RENDER STAGE
	GeometryRenderStage::ptr backgroundRender(new GeometryRenderStage("background"));
	backgroundRender->set_fbo_target(gbuffer->get_fbo());
	backgroundRender->set_camera(camera);
	backgroundRender->set_flag(RenderStageFlags::ClearColour | RenderStageFlags::ClearDepth);
	backgroundRender->set_clear_screen_colour(math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f));

	{
		GeometrySet::ptr geometrySet(new GeometrySet());
		backgroundRender->get_geometry()->add_geometry_set(geometrySet);
		geometrySet->insert(create_backplane(camera));
	}

	// --------------------------------- DISTANT GEOMETRY RENDER STAGE
	GeometryRenderStage::ptr distantStage(new GeometryRenderStage("distant geometry"));
	distantStage->set_fbo_target(gbuffer->get_fbo());
	distantStage->set_camera(camera);
	//distantStage->set_flag(RenderStageFlags::FrustumCull);
	distantStage->add_dependancy(backgroundRender);

	// --------------------------------- MAIN GEOMETRY RENDER STAGE
	GeometryRenderStage::ptr mainRender(new GeometryRenderStage("main geometry"));
	mainRender->set_fbo_target(gbuffer->get_fbo());
	mainRender->set_camera(camera);
	mainRender->set_flag(RenderStageFlags::FrustumCull);

	camera->setTransform(math::transform(Transform::vec3_type(0.0f, 0.0f, 0.0f), 
		Transform::vec3_type(0.0f, 0.0f, 10000.0f * WORLD_SCALE)));

	mainRender->add_dependancy(distantStage);

	// --------------------------------- LIGHTING STAGE
	FramebufferObject::ptr pBuffer(new FramebufferObject());
	Texture::ptr pTexture0(new Texture("pTexture0"));
	pTexture0->create2D(GL_TEXTURE_RECTANGLE, SDLGl::width(), SDLGl::height(), 4, GL_RGBA, GL_FLOAT, NULL, GL_RGBA32F);
	pBuffer->AttachTexture(pTexture0->handle(), GL_COLOR_ATTACHMENT0);

	LightingRenderStage::ptr lightingStage(new LightingRenderStage());

	static unsigned int shadowBufferSize = 1024;
	static unsigned int cascadeSplits = 4; // don't change, needs shader update...
	lightingStage->set_cascade_splits(cascadeSplits);
	lightingStage->set_p_buffer(pBuffer);
	lightingStage->set_flag(RenderStageFlags::ClearColour);
	lightingStage->set_geometry_stage(mainRender);

	FramebufferObject::ptr shadowingFBO(new FramebufferObject());

	lightingStage->set_shadow_fbo(shadowingFBO);

	transform::Group::ptr root(new transform::Group("root"));

	Texture::ptr shadowDepthBuffer(new Texture("shadowDepthBuffer"));
	shadowDepthBuffer->create3D(GL_TEXTURE_2D_ARRAY, shadowBufferSize, shadowBufferSize, cascadeSplits, 1, GL_DEPTH_COMPONENT, GL_FLOAT, NULL, GL_DEPTH_COMPONENT32F);
	shadowDepthBuffer->set_shadowing_parameters();
	lightingStage->set_cascade_shadow_depth_texture(shadowDepthBuffer);
	
	lightingStage->add_dependancy(mainRender);

	// --------------------------------- ATTENUATE STAGE
	scene::Material::ptr atmosphereAttenuateMaterial(new scene::Material());
	effect::Effect::ptr atmosphereAttenuateEffect(new effect::Effect());
	if(!atmosphereAttenuateEffect->load("../Data/Shaders/atmosphere_attenuate.xml"))
	{
		std::cout << "Error: " << atmosphereAttenuateEffect->get_last_error() << std::endl;
		return 1;
	}
	atmosphereAttenuateMaterial->set_effect(atmosphereAttenuateEffect);
	atmosphereAttenuateMaterial->set_parameter("PBuffer", pTexture0);
	atmosphereAttenuateMaterial->set_parameter("AlbedoBuffer", gbuffer->get_albedo());
	atmosphereAttenuateMaterial->set_parameter("PositionBufferTexture", gbuffer->get_position());

	FramebufferObject::ptr pBuffer2(new FramebufferObject());
	Texture::ptr pTexture2(new Texture("pTexture2"));
	pTexture2->create2D(GL_TEXTURE_RECTANGLE, SDLGl::width(), SDLGl::height(), 4, GL_RGBA, GL_FLOAT, NULL, GL_RGBA32F);
	pBuffer2->AttachTexture(pTexture2->handle(), GL_COLOR_ATTACHMENT0);

	render::GeometryRenderStage::ptr renderAtmosphereAttenuateBufferStage = render::utils::create_fsq_render_stage(SDLGl::width(), SDLGl::height(), atmosphereAttenuateMaterial, "atmosphere attenuate");
	renderAtmosphereAttenuateBufferStage->set_flag(RenderStageFlags::ClearColour);
	renderAtmosphereAttenuateBufferStage->set_clear_screen_colour(math::Vector4f(0, 0, 0, 1));
	renderAtmosphereAttenuateBufferStage->set_fbo_target(pBuffer2);
	renderAtmosphereAttenuateBufferStage->add_dependancy(lightingStage);

	// --------------------------------- SCATTERING STAGE
	effect::Effect::ptr atmosphereScatteringEffect(new effect::Effect());
	if(!atmosphereScatteringEffect->load("../Data/Shaders/atmosphere_scatter.xml"))
	{
		std::cout << "Error: " << atmosphereScatteringEffect->get_last_error() << std::endl;
		return 1;
	}

	render::GeometryRenderStage::ptr renderAtmosphereScatteringBufferStage = render::utils::create_2D_render_stage(SDLGl::width(), SDLGl::height(), "atmosphere scatter", pBuffer2);//(new render::GeometryRenderStage());
	renderAtmosphereScatteringBufferStage->set_geometry(render::ComposedGeometrySet::ptr(new render::ComposedGeometrySet()));
	renderAtmosphereScatteringBufferStage->add_dependancy(renderAtmosphereAttenuateBufferStage);

	// --------------------------------- EXPOSURE STAGE (FINAL WRITE TO BACK BUFFER)
	scene::Material::ptr renderPBufferMat(new scene::Material());
	effect::Effect::ptr renderPBufferEffect(new effect::Effect());

#if defined(RENDER_DEBUG_ENABLED)
	if(!renderPBufferEffect->load("../Data/Shaders/render_pbuffer_debug.xml"))
	{
		std::cout << "Error: " << renderPBufferEffect->get_last_error() << std::endl;
		return 1;
	}
	renderPBufferMat->set_parameter("AlbedoBuffer", gbuffer->get_albedo());
	renderPBufferMat->set_parameter("NormalBuffer", gbuffer->get_normal());
	renderPBufferMat->set_parameter("PositionBuffer", gbuffer->get_position());
	renderPBufferMat->set_parameter("DepthBuffer", gbuffer->get_depth());
	renderPBufferMat->set_parameter("PBuffer0", pTexture0);
	renderPBufferMat->set_parameter("DebugRenderMode", 0);
	int debugRenderMode = 0;
#else
	if(!renderPBufferEffect->load("../Data/Shaders/render_pbuffer.xml"))
	{
		std::cout << "Error: " << renderPBufferEffect->get_last_error() << std::endl;
		return 1;
	}
#endif

	renderPBufferMat->set_effect(renderPBufferEffect);
	renderPBufferMat->set_parameter("PBuffer", pTexture2);
	float exposure = 1.0f;
	renderPBufferMat->set_parameter("ExposureCoeff", exposure);

	render::GeometryRenderStage::ptr renderPBufferStage = render::utils::create_fsq_render_stage(SDLGl::width(), SDLGl::height(), renderPBufferMat, "render pbuffer");
	renderPBufferStage->add_dependancy(renderAtmosphereScatteringBufferStage);
	
	// --------------------------------- 3D OVERWRITE STAGE
	GeometryRenderStage::ptr ui3D(new GeometryRenderStage("3D ui"));
	ui3D->set_camera(camera);
	ui3D->add_dependancy(renderPBufferStage);

	// --------------------------------- UI OVERLAY STAGE
//#define UI_TEST
#if defined(UI_TEST)
	render::GeometryRenderStage::ptr renderUIOverlayStage = render::utils::create_ui_render_stage(SDLGl::width(), SDLGl::height(), "ui overlay");
	{
		glbase::Texture::ptr tex(new glbase::Texture("test ui texture"));
		tex->create2D(GL_TEXTURE_RECTANGLE, SDLGl::width(), SDLGl::height(), 4, GL_BGRA, GL_UNSIGNED_BYTE, NULL, GL_RGBA8);
		//CEFUI::set_ui("file://C:/Programming/src/Explore2/SourceData/UI/mainmenu/mainmenu.html", tex);
		effect::Effect::ptr uiEffect(new effect::Effect());
		uiEffect->load("../Data/Shaders/ui_overlay.xml");
		scene::Geometry::ptr uiGeom = render::utils::create_new_screen_quad(0, 0, SDLGl::width(), SDLGl::height());
		scene::Material::ptr uiMaterial(new scene::Material());
		uiMaterial->set_effect(uiEffect);
		uiMaterial->set_parameter("UITexture", tex);
		uiGeom->set_material(uiMaterial);
		render::GeometrySet::ptr geomSet(new render::GeometrySet());
		geomSet->insert(uiGeom);
		renderUIOverlayStage->get_geometry()->add_geometry_set(geomSet);
	}	
	renderUIOverlayStage->add_dependancy(ui3D);
#endif
	sceneContext->addStage(ui3D);

	// ------------------------------------------------------------------------------
	// setup planet and atmosphere

	effect::Effect::ptr solarLightEffect(new effect::Effect());
	if(!solarLightEffect->load("../Data/Shaders/solar_light.xml"))
	{
		std::cout << "Error: " << solarLightEffect->get_last_error() << std::endl;
		return 1;
	}

	auto scopedAtmospherics = Atmospherics::init(16, 16, 3);
	atmosphereAttenuateMaterial->set_parameter("OpticalDepthRayleighTexture", Atmospherics::get_optical_depth_rayleigh_textures());
	atmosphereAttenuateMaterial->set_parameter("OpticalDepthMieTexture", Atmospherics::get_optical_depth_mie_textures());

	Player::ptr player(new Player());
	player->set_camera(camera);
#if !defined(DISABLE_PLANET)

	SolarSystem::ptr solarSystem(new SolarSystem());
	solarSystem->init(mainRender, distantStage, lightingStage, renderAtmosphereScatteringBufferStage, ui3D, atmosphereAttenuateMaterial, solarLightEffect, atmosphereScatteringEffect, gbuffer);
	//solarSystem.load("../Data/Systems/test_system.xml");
	solarSystem->generate(0);
	player->set_solar_system(solarSystem);
 	Body::ptr planet = solarSystem->get_planets().empty()? Body::ptr() : solarSystem->get_planets().back(); //solarSystem.find_planet("earth");
 	if(planet)
 	{
 		solarSystem->update(camera, SDL_GetTicks(), Transform::vec3_type());
		camera->set_parent(planet->get_rotational_reference_frame().get());
		//player->change_frame_of_reference(planet);
 	}
#endif

	CHECK_OPENGL_ERRORS;

	glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
	bool quit = false;

	Uint32 t = SDL_GetTicks();
	//Transform::vec3_type velocity;
	bool lmb = false, mmb = false, rmb = false;
	bool frozen = false, linemode = false, gravity = true, collision = true;
	double speedscale(1.0);
	size_t ftime = 0;
	const Uint8 *keystate = SDL_GetKeyboardState(NULL);

	CHECK_OPENGL_ERRORS;
	cout << "Entering main loop" << endl;
	int lastFPSFrame = 0;
	for(int f = 1; !quit && f != abortFrames; ++f)
	{
		::utils::ProfileBlock frameProfile("frame", 100);

		while(RemoteDebug::get_instance()->process()) {}

		size_t frameTimeMS = SDL_GetTicks() - t;
		double ft(frameTimeMS / 1000.0);
		ftime += frameTimeMS;
		t = SDL_GetTicks();
		if(ftime > 100)
		{
			RemoteDebug::get_instance()->send_event(remote_utils::FrameTimeEvent(ftime, f - lastFPSFrame));
			lastFPSFrame = f;
			ftime = 0;

#if !defined(DISABLE_PLANET)
			//size_t oneWeightCount = 0, zeroWeightCount = 0;
			//remote_utils::ChunkLODStatsEvent::StatesVector chunkStates(5);
			//planet.apply_visitor_to_chunks(std::bind(chunk_lod_stats_visitor, 
			//	boost::ref(chunkStates), boost::ref(oneWeightCount), boost::ref(zeroWeightCount), std::placeholders::_1));

			//RemoteDebug::get_instance()->send_event(
			//	remote_utils::ChunkLODStatsEvent(chunkStates, 
			//	planet.get_curr_max_chunk_depth(), oneWeightCount));
#endif
		}

		player->update(ft);

#if !defined(DISABLE_PLANET)
		if(!frozen)
			solarSystem->update(camera, t, player->get_velocity());
#endif

		//Transform::matrix4_type cam(camera->transform());
		//Transform::vec4_type ctrans(cam.getColumnVector(3));
		//Transform::vec3_type lastPos(ctrans);
		//cam(0, 3) = Transform::float_type(0); cam(1, 3) = Transform::float_type(0); cam(2, 3) = Transform::float_type(0);
		//Transform::vec3_type forwardVec(camera->forwardGlobal());
		//Transform::vec3_type rightVec(camera->rightGlobal());
		
		SDL_Event event;
		SDL_PumpEvents();
		Transform::vec3_type velocityChange;
		if (keystate[SDL_SCANCODE_W] )
			velocityChange.z -= Transform::float_type(ft * speedscale * WORLD_SCALE);
		if (keystate[SDL_SCANCODE_S] )
			velocityChange.z += Transform::float_type(ft * speedscale * WORLD_SCALE);
		if (keystate[SDL_SCANCODE_D] )
			velocityChange.x += Transform::float_type(ft * speedscale * WORLD_SCALE);
		if (keystate[SDL_SCANCODE_A] )
			velocityChange.x -= Transform::float_type(ft * speedscale * WORLD_SCALE);
		player->accelerate_relative(velocityChange);

		//if (keystate[SDL_SCANCODE_BACKSPACE] )
		//{
		//	Transform::vec3_type vecSub = velocity.normal() * Transform::float_type(ft * speedscale * WORLD_SCALE);
		//	if(vecSub.lengthSquared() > velocity.lengthSquared())
		//		velocity = Transform::vec3_type::Zero;
		//	else
		//		velocity -= velocity.normal() * Transform::float_type(ft * speedscale * WORLD_SCALE);
		//}
		//ctrans += Transform::vec4_type(velocity * Transform::float_type(ft));

//#if !defined(DISABLE_PLANET)
//		Transform::matrix4_type cameraParentMatrix;
//		if(camera->parent())
//			cameraParentMatrix = camera->parent()->globalTransform();
//		ctrans = cameraParentMatrix * ctrans;
//		SolarSystem::DeferredCollision::ptr cameraCollision = solarSystem.collide(Transform::vec3_type(ctrans));
//#endif

		while(SDL_PollEvent(&event))
		{
			switch(event.type) {
			case SDL_KEYDOWN:
				switch(event.key.keysym.scancode)
				{
				case SDL_SCANCODE_ESCAPE: // esc
					quit = true; 
					break;
				case SDL_SCANCODE_SPACE: // space
					player->stop();
					//velocity = Transform::vec3_type();
					break;
				case SDL_SCANCODE_L: // L
					if(mainRender->is_flag_set(RenderStageFlags::WireFrame))
						mainRender->clear_flag(RenderStageFlags::WireFrame);
					else
						mainRender->set_flag(RenderStageFlags::WireFrame);
					break;
				case SDL_SCANCODE_F: // F
					frozen = !frozen;
					break;
				case SDL_SCANCODE_G: // G
					gravity = !gravity;
					break;
				case SDL_SCANCODE_C: // C
					collision = !collision;
					break;
#if defined(RENDER_DEBUG_ENABLED)
				case SDL_SCANCODE_M:
					debugRenderMode = (debugRenderMode+1) % DEBUG_RENDER_MODE_COUNT;
					renderPBufferMat->set_parameter("DebugRenderMode", debugRenderMode);
					break;
#endif
				case SDL_SCANCODE_1: 
				case SDL_SCANCODE_2: 
				case SDL_SCANCODE_3: 
				case SDL_SCANCODE_4: 
				case SDL_SCANCODE_5: 
				case SDL_SCANCODE_6: 
				case SDL_SCANCODE_7: 
				case SDL_SCANCODE_8: 
				case SDL_SCANCODE_9: 
				case SDL_SCANCODE_0: 
					speedscale = std::pow(10, event.key.keysym.scancode - SDL_SCANCODE_1); break;
				case SDL_SCANCODE_PAGEUP: // pgup
					exposure *= 0.9f;
					renderPBufferMat->set_parameter("ExposureCoeff", exposure);
					break;
				case SDL_SCANCODE_PAGEDOWN: // pgdn
					exposure *= 1.1f;
					renderPBufferMat->set_parameter("ExposureCoeff", exposure);
					break;
				default: 
					break ;
				};
				break;
			case SDL_MOUSEMOTION:
				if(lmb)
				{
					player->rotate_relative(Transform::vec3_type(event.motion.yrel/10.0, event.motion.xrel/10.0, 0.0));
					//cam.orthoNormalize();
				}
				if(mmb)
				{
					player->rotate_relative(Transform::vec3_type(0.0, 0.0, event.motion.xrel/10.0));
					//player->rotate(math::rotate_axis_angle(Transform::vec3_type(cam.getColumnVector(2)), Transform::float_type((event.motion.xrel)/10.0)));
					//cam.orthoNormalize();
				}
				//CEFUI::send_mouse_move_event(event.motion);
				break;
			case SDL_MOUSEBUTTONDOWN:
				if(event.button.button == SDL_BUTTON_LEFT)
					lmb = true;
				else if(event.button.button == SDL_BUTTON_MIDDLE)
					mmb = true;
				else if(event.button.button == SDL_BUTTON_RIGHT)
					rmb = true;
				//CEFUI::send_mouse_button_down_event(event.button);
				break;
			case SDL_MOUSEBUTTONUP:
				if(event.button.button == SDL_BUTTON_LEFT)
					lmb = false;
				else if(event.button.button == SDL_BUTTON_MIDDLE)
					mmb = false;
				else if(event.button.button == SDL_BUTTON_RIGHT)
					rmb = false;
				//CEFUI::send_mouse_button_up_event(event.button);
				break;
			case SDL_QUIT: 
				quit = true;	
				break;
			default: break ;
			};
		}


		SDLGl::swapBuffers();

 		update_materials();
		
		RenderGL::render(*sceneContext);

//#if !defined(DISABLE_PLANET)
//		if(collision)
//		{
//			ctrans = Transform::vec4_type(cameraCollision->get(), 1.0);
//			//const Transform* cameraParent = camera->parent();
//			//if(cameraParent)
//			//	newCameraPos = cameraParent->localise(newCameraPos);
//		}
//		ctrans = cameraParentMatrix.inverse() * ctrans;
//#endif
//
//		cam.setColumnVector(3, ctrans);
//		cam.orthoNormalize();
//		camera->setTransform(cam);

		stringstream s;
		unsigned int y = 0;

		s << "Acceleration scale: " << speedscale;
		RenderGL::draw_text(0, y, s.str(), font); y += 20;	s.str("");

		if(player->get_velocity().lengthSquared() > Transform::float_type(0))
		{
			s << "Velocity: " << double(player->get_velocity().length()) << " km/s";
			RenderGL::draw_text(0, y, s.str(), font); y += 20;	s.str("");
		}

		if(gravity)
		{
			s << "Gravity Enabled";
			RenderGL::draw_text(0, y, s.str(), font); y += 20;	s.str("");
		}

		if(collision)
		{
			s << "Collision Detection Enabled";
			RenderGL::draw_text(0, y, s.str(), font); y += 20;	s.str("");
		}

		std::vector<DataDistributeThread::DataBuilderThreadStats> buildStats = 
			DataDistributeThread::get_instance()->get_builder_thread_data();

		for(int idx = (int)buildStats.size() - 1; idx >= 0 ; --idx)
		{
			s << "buildthread " << idx << ": queued = " << buildStats[idx].dataQueued 
				<< ", work to do = " << buildStats[idx].totalWorkLeft;
			RenderGL::draw_text(0, y, s.str(), font); y += 20;	s.str("");
		}

		s << "Exposure: " << exposure;
		RenderGL::draw_text(0, y, s.str(), font); y += 20;	s.str("");

#if !defined(DISABLE_PLANET)
		//s << "Chunk depth: " << planet.get_curr_max_chunk_depth();
		//RenderGL::draw_text(0, y, s.str(), font); y += 20; s.str("");
#endif

#if defined(RENDER_DEBUG_ENABLED)
		s << "Render debug mode: " << get_debug_render_mode_name(debugRenderMode);
		RenderGL::draw_text(0, y, s.str(), font); y += 20; s.str("");
#endif
	}

	std::cout << "Shutting down rendering..." << std::endl;
	SDLGl::shutDownSDL();

	RemoteDebug::destroy();

	std::cout << "Waiting for data threads to terminate..." << std::endl;
	DataDistributeThread::destroy();

	std::cout << "Finished" << std::endl;
	return 0;
}

void chunk_lod_stats_visitor(remote_utils::ChunkLODStatsEvent::StatesVector& states, 
							 size_t& oneWeightCount,
							 size_t& zeroWeightCount,
							 ChunkLOD* chunk)
{
	++states[chunk->get_state()];
	if((ChunkLOD::State::Shown == chunk->get_state() || 
		ChunkLOD::State::PrepareChildren == chunk->get_state()))
	{
		if(chunk->get_blending_weight() == 1.0f)
			++oneWeightCount;
		else if(chunk->get_blending_weight() == 0.0f)
			++zeroWeightCount;
	}
}

// void update_atmosphere_material(scene::Material::ptr mat, 
// 								scene::transform::Camera::ptr camera, 
// 								scene::transform::Transform::ptr planetTransform)
// {
// 	mat->set_parameter("CameraToPlanetMatrix", 
// 		math::Matrix4f(planetTransform->globalTransformInverse() * camera->globalTransform()));
// 	math::Vector4d cameraPosGlobal(camera->globalTransform().getColumnVector(3));
// 	mat->set_parameter("CameraPlanetLocal", 
// 		math::Vector3f(planetTransform->globalTransformInverse() * cameraPosGlobal));
// }

//void update_material_planet_light_dir(scene::Material::ptr mat, 
//									  scene::transform::Light::ptr light,
//									  scene::transform::Transform::ptr planetTransform)
//{
//	//mat->set_parameter("CameraToPlanetMatrix", math::Matrix4f(planetTransform->globalTransformInverse() * camera->globalTransform()));
//	math::Vector4d lightDirGlobal(light->get_dir(), 0.0f);
//	mat->set_parameter("PlanetLightDir", 
//		math::Vector3f(planetTransform->globalTransformInverse() * lightDirGlobal));
//}

static std::vector< std::function<void()> > gMaterialUpdateFunctions;

void add_material_update(std::function< void() > fn)
{
	gMaterialUpdateFunctions.push_back(fn);
}

void update_materials()
{
	for(size_t idx = 0; idx < gMaterialUpdateFunctions.size(); ++idx)
	{
		(gMaterialUpdateFunctions[idx])();
	}
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
void APIENTRY gl_debug_callback_amd(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
	//GLenum source, GLenum type, GLuint id, GLenum severity,
	//			  GLsizei /*length*/, const GLchar* message, GLvoid* /*userParam*/)
{
	//char finalMessage[1024];
	//FormatDebugOutputARB(finalMessage, 1024, /*source, type,*/ id, severity, message);
	std::cout << "OpenGL error: " << message << std::endl;
}

std::ofstream glLog("GLLog.txt");
void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam)
{
	char finalMessage[1024];
	FormatDebugOutputARB(finalMessage, 1024, source, type, id, severity, message);
	std::cout << "OpenGL error: " << finalMessage << std::endl;
	glLog << finalMessage << std::endl;
}

scene::Geometry::ptr create_backplane(const scene::transform::Camera::ptr& camera)
{
	using namespace glbase;

	// setup back plane
	VertexSpec::ptr backPlaneVertexSpec(new VertexSpec());
	backPlaneVertexSpec->append(VertexData::PositionData, 0, sizeof(float), 3, VertexElementType::Float);
	VertexSet::ptr backPlaneVerts(new VertexSet(backPlaneVertexSpec, 4));
	double distance = 1;
	math::Vector3f backPlanebl(camera->unproject_global(math::Vector3d(0, 0, distance)));
	math::Vector3f backPlanebr(camera->unproject_global(math::Vector3d(0, SDLGl::height(), distance)));
	math::Vector3f backPlanetl(camera->unproject_global(math::Vector3d(SDLGl::width(), 0, distance)));
	math::Vector3f backPlanetr(camera->unproject_global(math::Vector3d(SDLGl::width(), SDLGl::height(), distance)));
	// top left vert
	*backPlaneVerts->extract<math::Vector3f>(0) = backPlanebl; 
	// bottom left vert
	*backPlaneVerts->extract<math::Vector3f>(1) = backPlanebr; 
	// top right
	*backPlaneVerts->extract<math::Vector3f>(2) = backPlanetl; 
	// bottom right
	*backPlaneVerts->extract<math::Vector3f>(3) = backPlanetr;
	backPlaneVerts->sync_all();

	// create the tri set for mesh
	TriangleSet::ptr backPlaneTriSet(new TriangleSet(TrianglePrimitiveType::TRIANGLE_STRIP));
	backPlaneTriSet->push_back(0);
	backPlaneTriSet->push_back(1);
	backPlaneTriSet->push_back(2);
	backPlaneTriSet->push_back(3);
	backPlaneTriSet->sync_all();
	scene::Material::ptr backPlaneMaterial(new scene::Material());
	effect::Effect::ptr backPlaneEffect(new effect::Effect());
	if(!backPlaneEffect->load("../Data/Shaders/background.xml"))
	{
		std::cout << "Error: " << backPlaneEffect->get_last_error() << std::endl;
		return scene::Geometry::ptr();
	}
	backPlaneMaterial->set_effect(backPlaneEffect);
	return scene::Geometry::ptr(new scene::Geometry(backPlaneTriSet, backPlaneVerts, backPlaneMaterial, camera));
}
