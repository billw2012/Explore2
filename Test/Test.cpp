// Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <limits>

#include "Effect/effect.h"

#include "Misc/filefuncs.hpp"

#include "GLbase/sdlgl.hpp"

#include "Math/matrix4.hpp"
#include "Math/transformation.hpp"

#include "Math/atmospheric.hpp"

// #ifdef WIN32
// #	pragma comment(lib, "opengl32.lib")
// #	pragma comment(lib, "glu32.lib")
// #	pragma comment(lib, "glew32.lib")
// #	ifdef _DEBUG
// #		pragma comment(lib, "SDLd.lib")
// #		pragma comment(lib, "SDLmaind.lib")
// #	else
// #		pragma comment(lib, "SDL.lib")
// #		pragma comment(lib, "SDLmain.lib")
// #	endif
// #endif

#undef max
#undef min

using namespace math;

typedef Atmospheref AtmosphereType;

template < class Ty_ > 
bool is_valid_float(Ty_ f)
{
	return f != std::numeric_limits<Ty_>::infinity() &&
		f != std::numeric_limits<Ty_>::quiet_NaN() &&
		f != std::numeric_limits<Ty_>::signaling_NaN() &&
		f != std::numeric_limits<Ty_>::denorm_min();
}

void write_image(const std::string& fileName, int ImageSize, const std::vector<AtmosphereType::spectrum_type>& results, std::vector<unsigned char> &imageData)
{
	imageData.resize(results.size() * 3);

	for(size_t idx2 = 0; idx2 < results.size(); ++idx2)
	{
		imageData[idx2*3+0] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2].x));
		imageData[idx2*3+1] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2].y));
		imageData[idx2*3+2] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2].z));
	}

	ilTexImage(ImageSize, ImageSize, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, &imageData[0]);
	iluFlipImage();
	{
		::DeleteFile(fileName.c_str());
		if(!ilSaveImage(fileName.c_str()))
		{
			std::cout << "error" << std::endl;
		}
	}
}

void write_image(const std::string& fileName, int ImageSize, const std::vector<AtmosphereType::float_type>& results, std::vector<unsigned char> &imageData, float scale = 1.0f)
{
	imageData.resize(results.size());

	for(size_t idx2 = 0; idx2 < results.size(); ++idx2)
	{
		imageData[idx2] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2] * scale));
	}

	ilTexImage(ImageSize, ImageSize, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, &imageData[0]);
	iluFlipImage();
	{
		::DeleteFile(fileName.c_str());
		if(!ilSaveImage(fileName.c_str()))
		{
			std::cout << "error" << std::endl;
		}
	}
}

void generate_eye_view( const std::string& fileName, const AtmosphereType &atmos, const AtmosphereType::spectrum_type& Is, const AtmosphereType::vec3_type& eyePos, const AtmosphereType::vec3_type& upDir, const AtmosphereType::vec3_type& sunDir, AtmosphereType::float_type mieg, const int ImageSize, std::vector<AtmosphereType::spectrum_type>& results, std::vector<AtmosphereType::spectrum_type>& results2, std::vector<unsigned char> &imageData ) 
{
	atmos.irradiance_at_view_Iv_table(Is, eyePos, upDir, sunDir, AtmosphereType::float_type(0), ImageSize, 5, 5, 5, results);
	atmos.irradiance_at_view_Iv_table(Is, eyePos, upDir, sunDir, mieg, ImageSize, 5, 5, 5, results2);

	for(size_t idx = 0; idx < results.size(); ++idx)
		results[idx] = AtmosphereType::float_type(1) - ::exp(-(results[idx] + results2[idx]) * AtmosphereType::float_type(2));
	write_image(fileName, ImageSize, results, imageData);
	//imageData.resize(results.size() * 3);

	//for(size_t idx2 = 0; idx2 < results.size(); ++idx2)
	//{
	//	assert(is_valid_float(results[idx2].x));
	//	assert(is_valid_float(results[idx2].y));
	//	assert(is_valid_float(results[idx2].z));
	//	imageData[idx2*3+0] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2].x));
	//	imageData[idx2*3+1] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2].y));
	//	imageData[idx2*3+2] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2].z));
	//}

	//ilTexImage(ImageSize, ImageSize, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, &imageData[0]);
	//iluFlipImage();
	//{
	//	::DeleteFile(fileName.c_str());
	//	if(!ilSaveImage(fileName.c_str()))
	//	{
	//		std::cout << "error" << std::endl;
	//	}
	//}
}

void generate_eye_views( int steps, const AtmosphereType::vec3_type& sunDir_, const AtmosphereType::mat4_type& rotMat, const AtmosphereType& atmos, const AtmosphereType::spectrum_type& Is, const AtmosphereType::vec3_type& eyePos, const AtmosphereType::vec3_type& upDir, const int ImageSize ) 
{
	std::vector<AtmosphereType::spectrum_type> results, results2;
	std::vector<unsigned char> imageData;

	AtmosphereType::float_type mieg = AtmosphereType::get_g(AtmosphereType::float_type(0.99));

	AtmosphereType::vec3_type sunDir = sunDir_;
	for(int idx = 1; idx <= steps; ++idx)
	{
		sunDir = AtmosphereType::vec3_type(rotMat * AtmosphereType::vec4_type(sunDir));

		{
			std::cout << "Generating eye view image " << idx << std::endl;
			std::stringstream ss;
			ss << "Images/Eye/eye_view_rayleigh_mie_" << idx << ".png";
			generate_eye_view(ss.str(), atmos, Is, eyePos, upDir, sunDir, mieg, ImageSize, results, results2, imageData);
		}

// 		{
// 			std::cout << "Generating eye view Mie image " << idx << std::endl;
// 			std::stringstream ss;
// 			ss << "Images/Eye/eye_view_mie_" << idx << ".png";
// 			generate_eye_view(ss.str(), atmos, Is, eyePos, upDir, sunDir, mieg, ImageSize, results, imageData);
// 		}	
	}
}

void generate_density_tests( AtmosphereType::float_type NsStart, AtmosphereType::float_type NsEnd, AtmosphereType::float_type mult, const AtmosphereType::vec3_type& sunDir, AtmosphereType atmos, const AtmosphereType::spectrum_type& Is, const AtmosphereType::vec3_type& eyePos, const AtmosphereType::vec3_type& upDir, const int ImageSize ) 
{
	std::vector<AtmosphereType::spectrum_type> results, results2;
	std::vector<unsigned char> imageData;

	AtmosphereType::float_type Ns = NsStart;
	for(int idx = 1; Ns < NsEnd; ++idx, Ns *= mult)
	{
		std::cout << "Generating inner integral table for Ns = " << Ns << std::endl;

		atmos.set_k(AtmosphereType::float_type(1.0003), Ns);
		atmos.set_light_wavelength(AtmosphereType::spectrum_type(AtmosphereType::float_type(0.000000700), AtmosphereType::float_type(0.000000550), AtmosphereType::float_type(0.000000460)));

		//atmos.calculate_inner_integral_table(20, ImageSize, ImageSize, results);
		//{
		//	std::stringstream ss;
		//	ss << "Images/Density/" << idx << "_optical_depth_" << Ns << ".png";
		//	write_image(ss.str(), ImageSize, results, imageData);
		//}
		//imageData.resize(results.size() * 3);

		//for(size_t idx2 = 0; idx2 < results.size(); ++idx2)
		//{
		//	imageData[idx2*3+0] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2].x));
		//	imageData[idx2*3+1] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2].y));
		//	imageData[idx2*3+2] = (unsigned char)(std::min(AtmosphereType::float_type(255), AtmosphereType::float_type(255) * results[idx2].z));
		//}

		//ilTexImage(ImageSize, ImageSize, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, &imageData[0]);
		//iluFlipImage();
		//{
		//	std::stringstream ss;
		//	ss << "Images/Density/" << idx << "_optical_depth_" << Ns << ".png";
		//	::DeleteFile(ss.str().c_str());
		//	if(!ilSaveImage(ss.str().c_str()))
		//	{
		//		std::cout << "error" << std::endl;
		//	}
		//}

		{
			std::cout << "Generating eye view image " << idx << std::endl;
			std::stringstream ss;
			ss << "Images/Density/" << idx << "_eye_view_rayleigh_mie.png";
			generate_eye_view(ss.str(), atmos, Is, eyePos, upDir, sunDir, 0, ImageSize, results, results2, imageData);
		}

// 		{
// 			std::cout << "Generating eye view Mie image " << idx << std::endl;
// 			std::stringstream ss;
// 			ss << "Images/Density/" << idx << "_eye_view_mei.png";
// 			generate_eye_view(ss.str(), atmos, Is, eyePos, upDir, sunDir, AtmosphereType::get_g(AtmosphereType::float_type(0.7)), ImageSize, results, imageData);
// 		}	
	}
}

int main(int argc, _TCHAR* argv[])
{
	using namespace math;

	const int ImageSize = 256;
	ilInit();
	ILuint ilhandle;
	ilGenImages(1, &ilhandle);
	ilBindImage(ilhandle);

	AtmosphereType atmos;

	atmos.set_radii(6378000, 6378000 + 300000);
	atmos.set_scale_height(300000.0f * 0.1f);
	AtmosphereType::float_type Ns = AtmosphereType::float_type(2e25);
	atmos.set_k(AtmosphereType::float_type(1.0003), Ns);
	atmos.set_light_wavelength(AtmosphereType::spectrum_type(AtmosphereType::float_type(0.000000700), AtmosphereType::float_type(0.000000550), AtmosphereType::float_type(0.000000460)));

	AtmosphereType::spectrum_type Is(AtmosphereType::float_type(10), AtmosphereType::float_type(10), AtmosphereType::float_type(10));
	AtmosphereType::vec3_type eyePos(AtmosphereType::float_type(6378100), AtmosphereType::float_type(0), AtmosphereType::float_type(0));
	AtmosphereType::vec3_type upDir(AtmosphereType::float_type(0), AtmosphereType::float_type(1), AtmosphereType::float_type(0));
	
	atmos.calculate_optical_depth_table(10, 128, 128);
	//generate_density_tests(8e24, 8e25, 1.1, AtmosphereType::vec3_type(-1, 0, 0), atmos, Is, eyePos, upDir, ImageSize);
  	//int steps = 100;
 	//generate_eye_views(steps, AtmosphereType::vec3_type(AtmosphereType::float_type(0), AtmosphereType::float_type(0), AtmosphereType::float_type(-1)), AtmosphereType::mat4_type(rotatey(180.0 / steps)), atmos, Is, eyePos, upDir, ImageSize);

	std::vector<AtmosphereType::spectrum_type> skyIlighting;
	std::vector<unsigned char> imageData;
	write_image("Images/planet_optical_depth.png", 128, atmos.get_optical_depth_table(), imageData, AtmosphereType::float_type(0.00002));
	atmos.calculate_surface_lighting_table(Is, AtmosphereType::float_type(0.9), 4, 4, 4, 128, 128, skyIlighting);
	write_image("Images/planet_skyI.png", 128, skyIlighting, imageData);

// 	AtmosphereType::float_type mieg = AtmosphereType::get_g(0.7);
// 	AtmosphereType::vec3_type res = atmos.irradiance_at_view_Iv(Is, AtmosphereType::vec3_type(0, 0, -1), eyePos, AtmosphereType::vec3_type(1, 0, 0), mieg, 5, 5, 5);

// 	std::vector<unsigned char> imageData(ImageSize * ImageSize * 3);
// 	for(size_t x = 0; x < ImageSize; ++x)
// 	{
// 		for(size_t y = 0; y < ImageSize; ++y)
// 		{
// 			imageData[y*ImageSize*3+x*3+0] = x * 256 / ImageSize;
// 			imageData[y*ImageSize*3+x*3+1] = y * 256 / ImageSize;
// 			imageData[y*ImageSize*3+x*3+2] = 0;
// 		}
// 	}
// 	ilTexImage(ImageSize, ImageSize, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, &imageData[0]);
// 	iluFlipImage();
// 	std::stringstream ss;
// 	ss << "Images/test.png";
// 	::DeleteFile("Images/test.png");
// 	if(!ilSaveImage("Images/test.png"))
// 	{
// 		std::cout << "error" << std::endl;
// 	}

	//AtmosphereType::spectrum_type Is(1000.0, 1000.0, 1000.0);
	//AtmosphereType::float_type theta = M_PI / 100;
	//AtmosphereType::vec2_type eyePos(6000000, 0);
	//AtmosphereType::vec2_type eyeDir(0, 1);
	//AtmosphereType::spectrum_type ii1 = atmos.inner_integral(0, M_PI / 2, 5);


// 	for(size_t idx = 0; idx < results.size(); ++idx)
// 	{
// 		results[idx].x = ::exp(results[idx].x);
// 		results[idx].y = ::exp(results[idx].y);
// 		results[idx].z = ::exp(results[idx].z);
// 	}
// 	for(size_t idx = 0; idx < results.size(); ++idx)
// 	{
// 		minVal = std::min(minVal, results[idx].x);
// 		minVal = std::min(minVal, results[idx].y);
// 		minVal = std::min(minVal, results[idx].z);
// 		maxVal = std::max(maxVal, results[idx].x);
// 		maxVal = std::max(maxVal, results[idx].y);
// 		maxVal = std::max(maxVal, results[idx].z);
// 	}
// 	for(size_t idx = 0; idx < results.size(); ++idx)
// 	{
// 		imageData[idx*3+0] = 255 * results[idx].x;
// 		imageData[idx*3+1] = 255 * results[idx].y;
// 		imageData[idx*3+2] = 255 * results[idx].z;
// 	}
// 	for(size_t x = 0; x < 256; ++x)
// 	{
// 		for(size_t y = 0; y < 256; ++y)
// 		{
// 			imageData[y*256*3+x*3+0] = x;
// 			imageData[y*256*3+x*3+1] = y;
// 			imageData[y*256*3+x*3+2] = 0;
// 		}
// 	}
// 	for(size_t idx = 0; idx < results.size(); ++idx)
// 	{
// 		imageData[idx*3+0] = 255 * (results[idx].x - minVal) / (maxVal - minVal);
// 		imageData[idx*3+1] = 255 * (results[idx].y - minVal) / (maxVal - minVal);
// 		imageData[idx*3+2] = 255 * (results[idx].z - minVal) / (maxVal - minVal);
// 	}
// 	for(size_t idx = 0; idx < results.size(); ++idx)
// 	{
// 		imageData[idx*3+0] = 255 * results[idx].x;
// 		imageData[idx*3+1] = 255 * results[idx].y;
// 		imageData[idx*3+2] = 255 * results[idx].z;
// 	}

	//AtmosphereType::spectrum_type ii1 = atmos.irradiance_at_view_Iv(Is, theta, eyePos, eyeDir, 5, 5, 5);
	//AtmosphereType::spectrum_type ii1 = atmos.inner_integral(AtmosphereType::vec2_type(6000000, 0), AtmosphereType::vec2_type(6010000, 0), M_PI / 2, 5, 5);
//  	glbase::SDLGl sdl;
//  
//  	sdl.initSDL();
//  	sdl.initOpenGL(1024, 768, 32);
//  
//  	GLenum err = glewInit();
//  	if (err != GLEW_OK)
//  	{
//  		std::cout << glewGetErrorString(err) << std::endl;
//  		return 1;
//  	}
//  
//  	effect::ShaderManager::init();

 
//  	effect::Effect eff;
//  	eff.load("../Data/Shaders/lod_chunk.xml");

	//effect::ShaderManager::get_instance()->get_shader("../Data/Shaders/test.glsl",
	//	effect::ShaderManager::ShaderType::VERTEX);
	//if(!eff.load("../Data/Shaders/testeff.xml"))
	//{
	//	std::cout << eff.get_last_error() << std::endl;
	//}

	// 3.5 1.5 .7
	return 0;
}

