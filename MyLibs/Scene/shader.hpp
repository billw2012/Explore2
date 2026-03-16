#ifndef _SCENE_SHADER_H
#define _SCENE_SHADER_H

#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <set>
#include <string.h>

#define BOOST_BIND_ENABLE_STDCALL
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/list/for_each_i.hpp>
#include <boost/preprocessor/list/size.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/comparison/less.hpp>

#include <GL/glew.h>
//#include <Cg/cg.h>
//#include <Cg/cgGL.h>

#include "Math/vector2.hpp"
#include "Math/vector3.hpp"
#include "Math/vector4.hpp"
#include "Math/matrix3.hpp"
#include "Math/matrix4.hpp"

#include "GLBase/texture.hpp"

#include "SceneDLL.h"

//#pragma comment(lib, "cg.lib")
//#pragma comment(lib, "cgGL.lib")

#ifdef _DEBUG
#define REMOVE_IN_RELEASE(x) x
#endif
#ifndef _DEBUG
#define REMOVE_IN_RELEASE(x)
#endif

#include "glbase/sdlgl.hpp"
#define START_TIMING Uint32 t = SDL_GetTicks();
#define END_TIMING Uint32 newt = SDL_GetTicks(); \
	float ft = (static_cast<float>(newt - t) / 1000.0f); \
	if(ft > 0.02f) \
	std::cout << "slow frame : " << ft << std::endl;

#define SHADER_PARAMETER_TYPE_COUNT		12
#define SHADER_PARAMETER_ELEMENT_COUNT	3

// Data on shader parameter types
//	| type							| elements	| setting macro
#define SHADER_PARAMETER_DATA \
	BOOST_PP_TUPLE_TO_LIST( \
	SHADER_PARAMETER_TYPE_COUNT, \
	( \
	( int ,							1 ,			SET_PARAMETER_1IR ), \
	( float ,						1 ,			SET_PARAMETER_1FR ), \
	( math::Vector2f ,				1 ,			SET_PARAMETER_2FR ), \
	( math::Vector3f ,				1 ,			SET_PARAMETER_3FR ), \
	( math::Vector4f ,				1 ,			SET_PARAMETER_4FR ), \
	( math::Matrix3f ,				1 ,			SET_MATRIX_PARAMETER_3FR ), \
	( math::Matrix4f ,				1 ,			SET_MATRIX_PARAMETER_4FR ), \
	( std::vector<float> ,			1 ,			SET_PARAMETER_ARRAY_1FR ), \
	( std::vector<math::Vector2f> ,	1 ,			SET_PARAMETER_ARRAY_2FR ), \
	( std::vector<math::Vector3f> ,	1 ,			SET_PARAMETER_ARRAY_3FR ), \
	( std::vector<math::Vector4f> ,	1 ,			SET_PARAMETER_ARRAY_4FR ), \
	( glbase::Texture::ptr ,		1 ,			SET_PARAMETER_TEXTURE ) \
	) \
	) \
	/**/

// set a float parameter (may have a number of elements)
#define SET_PARAMETER_1IR(_handle_ , _value_ , _count_)			\
	glUniform1iv( _handle_ , _count_ , _value_ )
#define SET_PARAMETER_1FR(_handle_ , _value_ , _count_)			\
	glUniform1fv( _handle_ , _count_ , reinterpret_cast<const float *>( & _value_ ) )
#define SET_PARAMETER_2FR(_handle_ , _value_ , _count_)			\
	glUniform2fv( _handle_ , _count_ , reinterpret_cast<const float *>( & _value_ ) )
#define SET_PARAMETER_3FR(_handle_ , _value_ , _count_)			\
	glUniform3fv( _handle_ , _count_ , reinterpret_cast<const float *>( & _value_ ) )
#define SET_PARAMETER_4FR(_handle_ , _value_ , _count_)			\
	glUniform4fv( _handle_ , _count_ , reinterpret_cast<const float *>( & _value_ ) )
#define SET_MATRIX_PARAMETER_3FR(_handle_ , _value_ , _count_)	\
	glUniformMatrix3fv( _handle_ , _count_ , false , reinterpret_cast<const float *>( & _value_ ) )
#define SET_MATRIX_PARAMETER_4FR(_handle_ , _value_ , _count_)	\
	glUniformMatrix4fv( _handle_ , _count_ , false , reinterpret_cast<const float *>( & _value_ ) )
// set an array parameter
#define SET_PARAMETER_ARRAY_1FR(_handle_ , _value_ , _count_)	\
	glUniform1fv( _handle_ , _count_ , reinterpret_cast<const float *>(& _value_ [0]) )
#define SET_PARAMETER_ARRAY_2FR(_handle_ , _value_ , _count_)	\
	glUniform2fv( _handle_ , _count_ , reinterpret_cast<const float *>(& _value_ [0]) )
#define SET_PARAMETER_ARRAY_3FR(_handle_ , _value_ , _count_)	\
	glUniform3fv( _handle_ , _count_ , reinterpret_cast<const float *>(& _value_ [0]) )
#define SET_PARAMETER_ARRAY_4FR(_handle_ , _value_ , _count_)	\
	glUniform4fv( _handle_ , _count_ , reinterpret_cast<const float *>(& _value_ [0]) )
// set a texture parameter
#define SET_PARAMETER_TEXTURE(_handle_ , _value_ , _count_)			\
	glUniform1i( _handle_ , _value_ ->handle() )

// extract the parameter type from the parameter data
#define PARAM_TYPE(elem)								BOOST_PP_TUPLE_ELEM(SHADER_PARAMETER_ELEMENT_COUNT, 0, elem)
// extract the parameter elements count from the parameter data
#define PARAM_ELEMENTS(elem)							BOOST_PP_TUPLE_ELEM(SHADER_PARAMETER_ELEMENT_COUNT, 1, elem)
// extract the parameter setting macro from the parameter data
#define PARAM_SET_FN(elem)								BOOST_PP_TUPLE_ELEM(SHADER_PARAMETER_ELEMENT_COUNT, 2, elem)

// comma separation macro
#define COMMA_EXPAND_PARAM_TYPE(r, data, i, elem)		BOOST_PP_COMMA_IF(i) PARAM_TYPE(elem)

// for each parameter in the parameter data
#define FOR_EACH_PARAM_TYPE_I(macro, data)				BOOST_PP_LIST_FOR_EACH_I(macro, data, SHADER_PARAMETER_DATA)
// output a comma separated list of the parameter types from the parameter data
#define ENUM_SHADER_PARAMETER_TYPES 					FOR_EACH_PARAM_TYPE_I(COMMA_EXPAND_PARAM_TYPE, NULL)

// define a () operator for the specified parameter type
#define DEFINE_SET_PARAM_OPERATOR(r, data, i, elem) \
	void operator()( const PARAM_TYPE( elem ) & value ) const \
	{ \
	PARAM_SET_FN( elem )( handle, value, PARAM_ELEMENTS( elem ) ) ; \
	} \
	/**/

// output a () operator setting function from each parameter type from the parameter data
#define ENUM_SHADER_PARAMETER_SET_OPERATORS				FOR_EACH_PARAM_TYPE_I(DEFINE_SET_PARAM_OPERATOR, NULL)


namespace scene
{

struct compile_error : public std::runtime_error
{
private:
	std::string _listing;
public:
	compile_error(std::string msg, std::string listing) : std::runtime_error(msg), _listing(listing) {}
	std::string getListing() { return _listing; }
	~compile_error()  _THROW0() {}
};

//SCENE_API void checkForCgError(const std::string& status, CGcontext context);

struct ShaderManager;

struct Shader
{
	friend struct ShaderManager;
	typedef boost::shared_ptr<Shader> ptr;

	typedef GLint parameter_handle;
	typedef boost::variant< 
		ENUM_SHADER_PARAMETER_TYPES
	> ParameterVariantType;

	struct Parameter : public ParameterVariantType
	{
	private:
		std::string _name;
		parameter_handle _currHandle;
	public:
		template < class T > 
		Parameter(const T& value, const std::string& name, parameter_handle handle)
			: ParameterVariantType(value),
			_name(name),
			_currHandle(handle)
		{
		}

		Parameter(const Parameter& other)
			: ParameterVariantType(static_cast<const ParameterVariantType&>(other)),
			_name(other._name),
			_currHandle(other._currHandle)
		{

		}

		void setName(const std::string& name) { _name = name; }
		void setHandle(parameter_handle handle) { _currHandle = handle; }
		template < class T >
		void setVal(const T& val)
		{
			// call the base classes assign method
			*static_cast<ParameterVariantType*>(this) = val;
		}

		const std::string& name() const { return _name; }
		parameter_handle handle() const { return _currHandle; }
	};

	//struct AutoMatrix
	//{
	//public:
	//	CGGLenum MatrixType;
	//	CGGLenum MatrixTransform;
	//	AutoMatrix() {};
	//	AutoMatrix(CGGLenum type, CGGLenum transform) : MatrixType(type), MatrixTransform(transform) {};
	//};

	//typedef std::map<parameter_handle, AutoMatrix> AutoMatrixMap;

	struct ParameterDesc
	{
		GLenum type;
		parameter_handle handle;
		std::string name;
		bool isArray;

		ParameterDesc(GLenum ptype = 0, parameter_handle hand = NULL, const std::string& pname = "", bool pisArray = false)
			: type(ptype), handle(hand), name(pname), isArray(/*pisArray*/ptype == CG_ARRAY)
		{
		}
	};

	typedef std::map<std::string, ParameterDesc> ParameterMap;
	typedef ParameterMap::iterator ParameterMapIterator;
	typedef ParameterMap::const_iterator ConstParameterMapIterator;

	typedef std::map<std::string, ParameterVariantType> ParameterNameValueMap; 
	typedef std::map<std::string, unsigned int> ParameterNameSizeMap;

	typedef std::map<std::string, std::string> SemanticMapType;
	typedef SemanticMapType::iterator SemanticIterator;
	typedef SemanticMapType::const_iterator ConstSemanticIterator;

private:
	typedef std::map<parameter_handle, glbase::Texture::ptr> TextureInputMap;
	typedef std::map<parameter_handle, float> FloatInputMap;
	typedef std::map<parameter_handle, std::vector<float> > FloatListInputMap;
	typedef std::map<parameter_handle, int> IntInputMap;
	typedef std::map<parameter_handle, math::Vector2f> Vector2fInputMap;
	typedef std::map<parameter_handle, std::vector<math::Vector2f> > Vector2fListInputMap;
	typedef std::map<parameter_handle, math::Vector3f> Vector3fInputMap;
	typedef std::map<parameter_handle, std::vector<math::Vector3f> > Vector3fListInputMap;
	typedef std::map<parameter_handle, math::Vector4f> Vector4fInputMap;
	typedef std::map<parameter_handle, std::vector<math::Vector4f> > Vector4fListInputMap;
	typedef std::map<parameter_handle, math::Matrix3f> Matrix3fInputMap;
	typedef std::map<parameter_handle, math::Matrix4f> Matrix4fInputMap;
	typedef std::map<parameter_handle, math::Matrix4d> Matrix4dInputMap;
	typedef std::set< std::pair< std::string, std::string > > MacroPairSet;
	
	//ShaderManager& _manager;

	// the Cg effect handle of the effect
	//GLuint _handle;
	// a copy of the Cg context handle this effect was loaded into
	ShaderManager* _manager;
	//CGcontext _contextHandle;
	// this is the user defined name of the effect (or the stripped out filename if no name was given)
	std::string _name;
	std::string _fileName;
	std::string _code;
	// if the effect is loaded using allTechniques flag then this contains the Cg technique handles 
	// of all valid techniques in the order they are specified in the file, 
	// otherwise it just contains the first valid technique
	//CGtechnique _technique;
	// this vector contains the Cg parameter handles for all the parameters in the effect
	ParameterMap _parameters;
	// current render pass
	//CGpass _pass;

	// cg parameters
	bool _valid;
	std::string _lastError;
	//AutoMatrixMap _autoMatrixMap;
	SemanticMapType _semanticMap;
	bool _transparency;

	bool _revalidationRequired;

	IntInputMap _intParameters;
	FloatInputMap _floatParameters;
	Vector2fInputMap _v2fParameters;
	Vector3fInputMap _v3fParameters;
	Vector4fInputMap _v4fParameters;
	FloatListInputMap _floatListParameters;
	Vector2fListInputMap _v2fListParameters;
	Vector3fListInputMap _v3fListParameters;
	Vector4fListInputMap _v4fListParameters;
	Matrix3fInputMap _m3fParameters;
	Matrix4fInputMap _m4fParameters;
	TextureInputMap _textureParameters;

	ParameterNameValueMap _literalParameters;
	ParameterNameSizeMap _arraySizes;
	//MacroPairSet _compilerMacros;

	// private shader, can only be created via the ShaderManager
	Shader(ShaderManager* manager, const std::string& fileName = "", const std::string& name = "");

public:
	void copyFrom(const Shader& from);

	~Shader();

	void setTransparency(bool transparency_);
	bool transparency() const;

	void destroy();

	CGcontext context() const;

	bool valid() const { return _valid; }

	// set a literal parameter
	template < class T >
	void setLiteralParameter( const std::string& name, const T& value );

	void setArrayParameterSize( const std::string& name, unsigned int size );

	void setCompilerMacro( const std::string& name, const std::string& value );

	const std::string& name() const;
	void setName(const std::string& name);

	// creates an effect from code, and parses out its technique(s) and parameters
	void create(const std::string& code);

	// loads an effect, and parses out its technique(s) and parameters
	void load(const std::string& fileName);

	// recompile all programs in shader technique using profiles from ShaderManager
	//void recompileTechnique();

	//void setProfileAndRecompile(CGprogram gprog, CGprofile bestgprof);

	//unsigned int techniqueCount() const { return _validTechniques.size(); }

	// returns the CGeffect handle of the encapsulated effect
	//CGeffect handle() const;

	ParameterMapIterator beginParameterMap();
	ConstParameterMapIterator beginParameterMap() const;

	ParameterMapIterator endParameterMap();
	ConstParameterMapIterator endParameterMap() const;

	const ParameterDesc& parameter(const std::string& name) const;

	ParameterDesc& parameter(const std::string& name);

	parameter_handle parameterHandle(const std::string& name) const;

	// attaches an OpenGL texture handle to the named parameter
	void bind(/*unsigned int technique*/);

	//void bindMatrices();

	//bool startPass();

	//void restartPass();

	std::string get_parameter_name_by_semantic(const std::string& str) const;

	SemanticIterator begin_semantics();
	SemanticIterator end_semantics();
	ConstSemanticIterator begin_semantics() const;
	ConstSemanticIterator end_semantics() const;

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const std::vector<float>& values);

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const std::vector<math::Vector2f>& values);
	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const std::vector<math::Vector3f>& values);

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const std::vector<math::Vector4f>& values);
	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, int value);
	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, float value);

	//void setParameter(const parameter_handle& parameterName, float value)
	//{
	//	FloatInputMap::iterator itr = _floatParameters.find(parameterName);
	//	if(itr == _floatParameters.end())
	//	{
	//		setParameter(parameterName, reinterpret_cast<const float *>(&value), 1, cgSetParameterValuefr);
	//		_floatParameters[parameterName] = value;
	//	}
	//	else if(itr->second != value)
	//	{
	//		setParameter(parameterName, reinterpret_cast<const float *>(&value), 1, cgSetParameterValuefr);
	//		itr->second = value;
	//	}
	//}

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const math::Vector2f& value);

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const math::Vector3f& value);

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const math::Vector4f& value);

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const math::Matrix3f& value);

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const math::Matrix4f& value);

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, const math::Matrix4d& value);

	template < class HandleType_ > 
	void setParameter(const HandleType_& parameterName, glbase::Texture::ptr texture);

private:

	template < class VSetTy_, class SetFnTy_ > 
	void setParameter(const std::string& name, const VSetTy_& valueset, SetFnTy_ fn);

	template < class VSetTy_, class SetFnTy_ > 
	void setParameter(parameter_handle handle, const VSetTy_& valueset, SetFnTy_ fn);

	template < class VTy_, class VSetTy_, class SetFnTy_, class SetTy_ > 
	void setParameter(parameter_handle handle, const VTy_& value, const VSetTy_& valueset, SetFnTy_ fn, SetTy_& valSet);

	template < class VTy_, class SetFnTy_ > 
	void setParameter(const std::string& name, const VTy_ *valueset, int nvals, SetFnTy_ fn);

	template < class VTy_, class SetFnTy_ > 
	void setParameter(parameter_handle handle, const VTy_ *valueset, int nvals, SetFnTy_ fn);

	template < class VTy_, class VSetTy_, class SetFnTy_, class SetTy_ > 
	void setParameter(parameter_handle handle, const VTy_& value, const VSetTy_ *valueset, int nvals, SetFnTy_ fn, SetTy_& valSet);

	//bool loadTechniques();

	//void parseParameters();

	void applyLiteralParameter( parameter_handle param, const ParameterVariantType& literalParam );

	void applyArraySize( parameter_handle param, unsigned int size );
};

//inline void cgErrorHandler(CGcontext context, CGerror error, void* appData)
//{
//	const char *err = cgGetErrorString(error);
//	if(error == CG_COMPILER_ERROR)
//	{
//		const char *compileErr = cgGetLastListing(context);
//		std::string compileError(compileErr);
//		throw compile_error("cgErrorHandler", std::string(err) + ":\n" + compileError);
//	}
//	else
//		throw std::runtime_error(std::string(err) + ":\n cgErrorHandler");
//}

struct ShaderManager
{
private:
	SCENE_API static boost::shared_ptr<ShaderManager> _instance;
	CGcontext _contextHandle;
	Shader::ptr _currentShader;
	CGprofile _geometryProfile, _vertexProfile, _fragmentProfile;

protected:
	ShaderManager()
	{
		_contextHandle = cgCreateContext();
		checkForCgError("init", _contextHandle);
		//cgSetParameterSettingMode(_contextHandle, CG_DEFERRED_PARAMETER_SETTING);
		cgGLRegisterStates(_contextHandle);
		cgGLSetManageTextureParameters(_contextHandle, CG_TRUE);
		checkForCgError("setup", _contextHandle);

		_geometryProfile = cgGLGetLatestProfile(CG_GL_GEOMETRY);
		_vertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
		_fragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);

		cgSetAutoCompile( _contextHandle, CG_COMPILE_LAZY );

		//cgSetErrorHandler(cgErrorHandler, NULL);
	}

public:
	static ShaderManager& globalManager()
	{
		if(_instance.get() == NULL)
			_instance.reset(new ShaderManager());
		return *_instance.get();
	}

	CGcontext handle() const { return _contextHandle; }

	Shader::ptr createShader(const std::string& name, const std::string& fileName = "")
	{
		Shader::ptr shader(new Shader(this, fileName, name));
		shader->setName(name);
		return shader;
	}

	void setGeometryProfile(CGprofile profile)
	{
		_geometryProfile = profile;
	}

	CGprofile getGeometryProfile()
	{
		return _geometryProfile;
	}

	void setVertexProfile(CGprofile profile)
	{
		_vertexProfile = profile;
	}

	CGprofile getVertexProfile()
	{
		return _vertexProfile;
	}

	void setFragmentProfile(CGprofile profile)
	{
		_fragmentProfile = profile;
	}

	CGprofile getFragmentProfile()
	{
		return _fragmentProfile;
	}

//  private:
//  	void bind(Shader::ptr shader)
//  	{
//  		if(_currentShader != shader)
//  		{
//  			_currentShader = shader;
//  			shader->bind();
//  		}
//  	}
};

#include "shader.inl"

}

#endif // _SCENE_SHADER_H
