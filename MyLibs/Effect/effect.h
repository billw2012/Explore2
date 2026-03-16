#if !defined(__EFFECT_EFFECT_H__)
#define __EFFECT_EFFECT_H__

#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <boost/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/filesystem/path.hpp>

#include "Math/vector2.hpp"
#include "Math/vector3.hpp"
#include "Math/vector4.hpp"
#include "Math/matrix3.hpp"
#include "Math/matrix4.hpp"

#include "GLBase/texture.hpp"

#include "shadermanager.h"

#include "glstate.h"

class TiXmlElement;

#pragma warning(push)
#pragma warning(disable: 4251)

namespace effect {;

struct Effect
{
	struct EffectMode { enum type {
		Render,
		Shadow
	};};

	typedef std::shared_ptr< Effect > ptr;

	typedef boost::variant < 
		int, float, math::Vector2f, math::Vector3f,
		math::Vector4f, math::Matrix3f, math::Matrix4f,	
		std::vector<int>, std::vector<float>, std::vector<math::Vector2f>,	
		std::vector<math::Vector3f>, std::vector<math::Vector4f>,
		std::vector<math::Matrix3f>, std::vector<math::Matrix4f>,
		glbase::Texture::ptr > parameter_variant_type;
#if _MSC_VER >= 1700
	struct ParameterVariantType : parameter_variant_type
	{
		ParameterVariantType() {}

		template <typename T>
		ParameterVariantType(const T& operand) : parameter_variant_type(operand)
		{
			//convert_construct(operand, 1L);
		}

		template <typename T>
		ParameterVariantType(T& operand) : parameter_variant_type(operand)
		{
			//convert_construct(operand, 1L);
		}
	};
#else
	typedef parameter_variant_type ParameterVariantType;
#endif

	Effect();
	~Effect();

	bool load(const boost::filesystem::path& file);
	bool reload();

	static void reload_all();

	void set_state(EffectMode::type mode = EffectMode::Render) const;
	//void reset_state() const;

	void bind(EffectMode::type mode = EffectMode::Render) const;
	static void unbind();
	static GLint set_texture(GLenum target, GLuint handle);
	static void bind_texture(GLint index);

	bool has_shadow_mode() const { return _shadowProgram != NULL; }
 
	const std::string& get_last_error() const;

	void set_parameter(const std::string& param, const ParameterVariantType& val, EffectMode::type mode = EffectMode::Render);

	template < class Fn >
	void for_each_symantic(Fn op) const;

	bool validate_program() const;

	static void reset_state();

	struct ParameterHandle
	{
		std::string name;
		GLint handle;

		ParameterHandle(const std::string& name_ = std::string()) : handle(-1), name(name_) {}
	};

	//typedef GLuint ParameterHandle;
	typedef GLuint ProgramHandle;
private:
	typedef std::unordered_set<std::string> StringSet;
	typedef std::unordered_map<std::string, StringSet> StringStringSetMap;
	typedef std::vector<ParameterHandle> ParamHandleSet;
	typedef std::unordered_map<std::string, ParamHandleSet> StringParamHandleSetMap;


	bool load_program_node(TiXmlElement* node,std::string& fileName,
		StringStringSetMap& paramMap, const std::string& cwd);
	void map_parameters(StringParamHandleSetMap& paramMap, const StringStringSetMap& namesMap, 
		ShaderManager::ShaderHandle prog);
	void parse_state( TiXmlElement* stateNode, GLState::GLStateUniqueSet& stateVec );

	bool load_internal(const boost::filesystem::path& file, bool force);

	struct ShaderLoadErrorType { enum type {
		NoError = 0,
		NoNode,
		LoadError
	};};
	ShaderLoadErrorType::type load_shader(TiXmlElement* root, const char* elemName,
		ShaderManager::LoadFlags shaderType, const std::string& fullDir, ProgramHandle& program,
		StringStringSetMap& paramMap);

	static void apply_state(const GLState::GLStateUniqueSet& state);

	void show_last_error_if_debug() const;

private:
	std::string _name;
	boost::filesystem::path _file;
	ProgramHandle _program, _shadowProgram;
	StringParamHandleSetMap _paramMap, _shadowParamMap;
	mutable std::string _lastError;
	GLState::GLStateUniqueSet _stateSet, _shadowStateSet;
	mutable EffectMode::type _boundStateType;

	struct BoundTexture
	{
		BoundTexture() {}
		BoundTexture(GLuint unit_) : target(0), handle(0), unit(unit_) {}
		void set(GLenum target_, GLuint handle_)
		{
			target = target_;
			handle = handle_;
		}
		void bind()
		{
			glActiveTexture(GL_TEXTURE0 + unit);
// 			glBindTexture(GL_TEXTURE_1D, 0);
// 			glBindTexture(GL_TEXTURE_2D, 0);
// 			glBindTexture(GL_TEXTURE_RECTANGLE, 0);
// 			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
// 			glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, 0);
			//if(target != target_ && target != 0)
			//{
			//	glBindTexture(target, 0);
			//}
			glBindTexture(target, handle);
		}
		void unbind()
		{
			if(target != 0 && handle != 0)
			{
				glActiveTexture(GL_TEXTURE0 + unit);
				glBindTexture(target, 0);
				handle = target = 0;
			}
		}
		GLenum target;
		GLuint handle, unit;
	};

	static std::vector<BoundTexture> _activeTextures;
	static size_t _activeTexturesCount;
	static ProgramHandle _boundShader;
	static GLState::GLStateSet _currentState;
};

template < class Fn >
void Effect::for_each_symantic(Fn op) const
{
	for(StringParamHandleSetMap::const_iterator cItr = _paramMap.begin(); cItr != _paramMap.end(); ++cItr)
		op(cItr->first);
}

}

#pragma warning(pop)

#endif // __EFFECT_EFFECT_H__