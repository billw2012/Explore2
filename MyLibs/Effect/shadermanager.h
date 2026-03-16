#if !defined( __EFFECT_PROGRAM_MANAGER_H__ )
#define __EFFECT_PROGRAM_MANAGER_H__

#include <GL/glew.h>

#include <string>

#include <memory>

#include <boost/variant.hpp>

#include "Math/vector2.hpp"
#include "Math/vector3.hpp"
#include "Math/vector4.hpp"

#pragma warning(push)
#pragma warning(disable: 4251)

namespace effect {;

struct ShaderManager
{
	typedef GLuint ShaderHandle;

	enum LoadFlags { 
		VERTEX			= 1 << 0, 
		FRAGMENT		= 1 << 1,
		SHADER_TYPE_MASK = VERTEX | FRAGMENT,
		FORCE_RELOAD	= 1 << 2,
		NON				= 0
	};

	typedef boost::variant < int, float, math::Vector2f, math::Vector3f, math::Vector4f > define_variant_type;
	static void set_define(const std::string& name, const define_variant_type& value);

	static ShaderHandle get_shader(const std::string& prog, LoadFlags pt);

	static void reload_shaders();

	static const std::string& get_last_listing();
};

}


inline effect::ShaderManager::LoadFlags operator|(const effect::ShaderManager::LoadFlags& lhs, const effect::ShaderManager::LoadFlags& rhs)
{
	return static_cast<effect::ShaderManager::LoadFlags>(static_cast<size_t>(lhs) | static_cast<size_t>(rhs));
}

inline effect::ShaderManager::LoadFlags operator&(const effect::ShaderManager::LoadFlags& lhs, const effect::ShaderManager::LoadFlags& rhs)
{
	return static_cast<effect::ShaderManager::LoadFlags>(static_cast<size_t>(lhs) & static_cast<size_t>(rhs));
}

#pragma warning(pop)

#endif // __EFFECT_PROGRAM_MANAGER_H__