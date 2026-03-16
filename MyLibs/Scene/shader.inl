inline Shader::Shader(ShaderManager* manager, const std::string& fileName, const std::string& name)
	: _manager(manager),
	_handle(NULL),
	_name(name),
	_valid(false),
	_transparency(false),
	_pass(NULL),
	_fileName(fileName),
	_revalidationRequired(true)
{
	if(!fileName.empty())
		load(fileName);
}

inline void Shader::copyFrom(const Shader& from)
{
	destroy();

	_transparency = from._transparency;

	if(from._fileName != "")
		load(from._fileName);
	else if(from._code != "")
		create(from._code);
}

inline Shader::~Shader()
{
	destroy();
}

inline void Shader::setTransparency(bool transparency_) { _transparency = transparency_; }
inline bool Shader::transparency() const { return _transparency; }

inline void Shader::destroy()
{
	if(_handle != NULL)
	{
		cgDestroyEffect(_handle);
		checkForCgError("destroying effect", context());
		_handle = NULL;
		_code = "";
		_fileName = "";
		_valid = false;
		_pass = NULL;
	}
}

inline CGcontext Shader::context() const 
{ 
	return _manager->handle(); 
}

// set a literal parameter
template < class T >
inline void Shader::setLiteralParameter( const std::string& name, const T& value )
{
	//if(_handle != NULL)
	//	throw std::runtime_error ( "Set literal parameters before loading a shader!" );

	ParameterNameValueMap::const_iterator iItr = _literalParameters.insert(ParameterNameValueMap::value_type(name, ParameterVariantType(value))).first;

	if(_handle != NULL)
	{
		ConstParameterMapIterator pItr = _parameters.find(name);
		if(pItr == _parameters.end())
			throw std::runtime_error(std::string("Could not find literal parameter ") + name + std::string(" in shader ") + _name);
		applyLiteralParameter(pItr->second.handle, iItr->second);
	}
}

inline void Shader::setArrayParameterSize( const std::string& name, unsigned int size )
{
	//if(_handle != NULL)
	//	throw std::runtime_error ( "Set array parameter size before loading a shader!" );

	_arraySizes.insert(ParameterNameSizeMap::value_type(name, size));

	if(_handle != NULL)
	{
		ConstParameterMapIterator pItr = _parameters.find(name);
		if(pItr == _parameters.end())
			throw std::runtime_error(std::string("Could not find array parameter ") + name + std::string(" in shader ") + _name);

		if(!pItr->second.isArray)
			throw std::runtime_error(std::string("Parameter ") + name + std::string(" in shader ") + _name + std::string(" is not an array"));
		applyArraySize(pItr->second.handle, size);
	}
}

inline void Shader::setCompilerMacro( const std::string& name, const std::string& value )
{
	if(_handle != NULL)
		throw std::runtime_error("Cannot set compiler macros after a shader has been loaded");
	_compilerMacros.insert(MacroPairSet::value_type(name, value));
}

inline const std::string& Shader::name() const { return _name; }
inline void Shader::setName(const std::string& name) { _name = name; }

// creates an effect from code, and parses out its technique(s) and parameters
inline void Shader::create(const std::string& code)
/*throw(compile_error, std::runtime_error)*/
{
	destroy();

	_handle = cgCreateEffect(context(), code.c_str(), NULL);

	//const char *last = cgGetLastListing(context());
	checkForCgError("creating effect", context());

	if(!loadTechniques())
		throw std::runtime_error("No valid techniques found!");

	_code = code;
}

// calls a function when it is destroyed
template < class Fn >
struct ScopedFunctionCall
{
	Fn fn;

	ScopedFunctionCall(Fn fn_) : fn(fn_) {}

	~ScopedFunctionCall() { fn(); }
};

// loads an effect, and parses out its technique(s) and parameters
inline void Shader::load(const std::string& fileName)
/*throw(compile_error, std::runtime_error)*/
{
	_valid = false;

	destroy();

	//CGenum autocompile = cgGetAutoCompile( context() );

	//ScopedFunctionCall< boost::function< void () > > resetAutoCompile( boost::bind(cgSetAutoCompile, context(), autocompile) );

	//cgSetAutoCompile( context(), CG_COMPILE_MANUAL );

	// set compiler macros
	boost::shared_array<char *> macros( new char*[_compilerMacros.size()+1] );
	std::vector< boost::shared_array< char > > macroElements;
	unsigned int idx = 0;
	for(MacroPairSet::const_iterator mItr = _compilerMacros.begin(); mItr != _compilerMacros.end(); ++mItr, ++idx)
	{
		std::string macroString("-D" + mItr->first + "=" + mItr->second);
		macroElements.push_back( boost::shared_array< char >( new char[macroString.length() + 1] ) );
		//macros[idx] = new char[ macroString.length() + 1 ];
		macros[idx] = macroElements.back().get();
		strcpy_s( macros[idx], macroString.length(), macroString.c_str() );
	}
	macros[idx] = NULL;

	_handle = cgCreateEffectFromFile(context(), fileName.c_str(), const_cast<const char **>(macros.get()));

	checkForCgError("loading effect", context());

	if(!loadTechniques())
		throw std::runtime_error("No valid techniques found!");

	_fileName = fileName;

	_valid = true;
}

inline void Shader::recompileTechnique()
{
	if(_technique == NULL)
		return;

	//CGprofile gprof = _manager->getGeometryProfile();//cgGLGetLatestProfile(CG_GL_GEOMETRY);
	//CGprofile vprof = _manager->getVertexProfile();//cgGLGetLatestProfile(CG_GL_VERTEX);
	//CGprofile fprof = _manager->getFragmentProfile();//cgGLGetLatestProfile(CG_GL_FRAGMENT);

	//CGpass pass = cgGetFirstPass(_technique);
	//while(pass != NULL)
	//{
	//	//CGprogram gprog = cgGetPassProgram(pass, CG_GEOMETRY_DOMAIN);
	//	CGprogram vprog = cgGetPassProgram(pass, CG_VERTEX_DOMAIN);
	//	CGprogram fprog = cgGetPassProgram(pass, CG_FRAGMENT_DOMAIN);

	//	//if(gprog != NULL)
	//	//	setProfileAndRecompile(gprog, gprof);
	//	if(vprog != NULL)
	//		setProfileAndRecompile(vprog, vprof);
	//	if(fprog != NULL)
	//		setProfileAndRecompile(fprog, fprof);

	//	pass = cgGetNextPass(pass);
	//}	
}

inline void Shader::setProfileAndRecompile(CGprogram gprog, CGprofile bestgprof)
{
	cgSetProgramProfile(gprog, bestgprof);
	checkForCgError("setting program profile to best available", context());
	cgCompileProgram(gprog);
	checkForCgError("recompiling program", context());
}

//unsigned int techniqueCount() const { return _validTechniques.size(); }

// returns the CGeffect handle of the encapsulated effect
inline CGeffect Shader::handle() const {	return _handle;	}

inline Shader::ParameterMapIterator Shader::beginParameterMap() { return _parameters.begin(); }
inline Shader::ConstParameterMapIterator Shader::beginParameterMap() const { return _parameters.begin(); }

inline Shader::ParameterMapIterator Shader::endParameterMap() { return _parameters.end(); }
inline Shader::ConstParameterMapIterator Shader::endParameterMap() const { return _parameters.end(); }

inline const Shader::ParameterDesc& Shader::parameter(const std::string& name) const 
{
	ConstParameterMapIterator itr = _parameters.find(name);
	//if(itr != _parameters.end())
	return itr->second;
	//else
	//	return NULL;
}

inline Shader::ParameterDesc& Shader::parameter(const std::string& name) 
{
	ParameterMapIterator itr =_parameters.find(name);
	//if(itr != _parameters.end())
	return itr->second;
	//else
	//	return NULL;
}

inline CGparameter Shader::parameterHandle(const std::string& name) const 
{
	ConstParameterMapIterator itr =_parameters.find(name);
	if(itr != _parameters.end())
		return itr->second.handle;
	else
		return NULL;
}

inline std::string Shader::get_parameter_name_by_semantic(const std::string& str) const
{
	ConstSemanticIterator sItr = _semanticMap.find(str);
	if(sItr != _semanticMap.end())
		return sItr->second;
	return std::string();
}

inline Shader::SemanticIterator Shader::begin_semantics()
{
	return _semanticMap.begin();
}

inline Shader::SemanticIterator Shader::end_semantics()
{
	return _semanticMap.end();
}

inline Shader::ConstSemanticIterator Shader::begin_semantics() const
{
	return _semanticMap.begin();
}

inline Shader::ConstSemanticIterator Shader::end_semantics() const
{
	return _semanticMap.end();
}

// attaches an OpenGL texture handle to the named parameter
inline void Shader::bind(/*unsigned int technique*/)
{
	//_techniqueNumber = technique;
	_pass = NULL;

	if(_revalidationRequired)
	{
		if(cgValidateTechnique(_technique) != CG_TRUE)
			throw std::runtime_error("could not re-validate technique");
		_revalidationRequired = false;
	}

	//bindMatrices();
	//attachSymanticParameters();
}

//void Shader::bindMatrices()
//{
//	// bind automatic matrix parameters
//	for(AutoMatrixMap::iterator mItr = _autoMatrixMap.begin(); mItr != _autoMatrixMap.end(); mItr++)
//	{
//		cgGLSetStateMatrixParameter(mItr->first, mItr->second.MatrixType, mItr->second.MatrixTransform);
//		checkForCgError("loading matrix", context());
//	}
//}

inline bool Shader::startPass()
{
	if(_pass == NULL)
	{
		if((_pass = cgGetFirstPass(_technique)) == NULL)
			return false;
		checkForCgError("getting first pass", context());
	}
	else
	{
		cgResetPassState(_pass);
		checkForCgError("resetting pass state", context());
		if((_pass = cgGetNextPass(_pass)) == NULL)
			return false;
		checkForCgError("getting next pass", context());
	}
	cgSetPassState(_pass);
	checkForCgError("setting pass state", context());
	return true;
}

inline void Shader::restartPass()
{
	_pass = NULL;
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const std::vector<float>& values)
{
	setParameter(parameterName, values, reinterpret_cast<const float *>(&values[0]), static_cast<int>(values.size()), cgSetParameterValuefr, _floatListParameters);		
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const std::vector<math::Vector2f>& values)
{
	setParameter(parameterName, values, reinterpret_cast<const float *>(&values[0]), static_cast<int>(values.size() * 2), cgSetParameterValuefr, _v2fListParameters);		
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const std::vector<math::Vector3f>& values)
{
	setParameter(parameterName, values, reinterpret_cast<const float *>(&values[0]), static_cast<int>(values.size() * 3), cgSetParameterValuefr, _v3fListParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const std::vector<math::Vector4f>& values)
{
	setParameter(parameterName, values, reinterpret_cast<const float *>(&values[0]), static_cast<int>(values.size() * 4), cgSetParameterValuefr, _v4fListParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, int value)
{
	setParameter(parameterName, value, reinterpret_cast<const int *>(&value), 1, cgSetParameterValueir, _intParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, float value)
{
	setParameter(parameterName, value, reinterpret_cast<const float *>(&value), 1, 
		cgSetParameterValuefr, _floatParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const math::Vector2f& value)
{
	setParameter(parameterName, value, reinterpret_cast<const float *>(&value.x), 2, cgSetParameterValuefr, _v2fParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const math::Vector3f& value) 
{
	setParameter(parameterName, value, reinterpret_cast<const float *>(&value.x), 3, cgSetParameterValuefr, _v3fParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const math::Vector4f& value)
{
	setParameter(parameterName, value, reinterpret_cast<const float *>(&value.x), 4, cgSetParameterValuefr, _v4fParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const math::Matrix3f& value)
{		
	setParameter(parameterName, value, reinterpret_cast<const float *>(&value.m11), 9, cgSetParameterValuefr, _m3fParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const math::Matrix4f& value)
{
	setParameter(parameterName, value, reinterpret_cast<const float *>(&value.m11), 16, cgSetParameterValuefr, _m4fParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, const math::Matrix4d& value)
{
	setParameter(parameterName, value, reinterpret_cast<const double *>(&value.m11), 16, cgSetParameterValuedr, _m4dParameters);
}

template < class HandleType_ > 
void Shader::setParameter(const HandleType_& parameterName, glbase::Texture::ptr texture)
{
	if(texture != NULL)
		setParameter(parameterName, texture, texture->handle(), cgGLSetTextureParameter, _textureParameters);
}

template < class VSetTy_, class SetFnTy_ > 
void Shader::setParameter(const std::string& name, const VSetTy_& valueset, SetFnTy_ fn)
{
	CGparameter param = parameterHandle(name);
	if(param != NULL)
	{
		fn(param, valueset);
		REMOVE_IN_RELEASE(checkForCgError("setting param", context()));
	}
}

template < class VSetTy_, class SetFnTy_ > 
void Shader::setParameter(parameter_handle handle, const VSetTy_& valueset, SetFnTy_ fn)
{
	if(handle != NULL)
	{
		fn(handle, valueset);
		REMOVE_IN_RELEASE(checkForCgError("setting param", context()));
	}
}

template < class VTy_, class VSetTy_, class SetFnTy_, class SetTy_ > 
void Shader::setParameter(parameter_handle handle, const VTy_& value, const VSetTy_& valueset, SetFnTy_ fn, SetTy_& valSet)
{
	if(handle != NULL)
	{
		SetTy_::iterator itr = valSet.find(handle);
		if(itr == valSet.end())
		{
			fn(handle, valueset);
			REMOVE_IN_RELEASE(checkForCgError("setting param", context()));
			valSet[handle] = value;
		}
		else if(itr->second != value)
		{
			fn(handle, valueset);
			REMOVE_IN_RELEASE(checkForCgError("setting param", context()));
			itr->second = value;
		}
	}
}

template < class VTy_, class SetFnTy_ > 
void Shader::setParameter(const std::string& name, const VTy_ *valueset, int nvals, SetFnTy_ fn)
{
	CGparameter param = parameterHandle(name);
	if(param != NULL)
	{
		fn(param, nvals, valueset);
		REMOVE_IN_RELEASE(checkForCgError(name, context()));
	}
}

template < class VTy_, class SetFnTy_ > 
void Shader::setParameter(parameter_handle handle, const VTy_ *valueset, int nvals, SetFnTy_ fn)
{
	if(handle != NULL)
	{
		fn(handle, nvals, valueset);
		REMOVE_IN_RELEASE(checkForCgError("setting param", context()));
	}
}

template < class VTy_, class VSetTy_, class SetFnTy_, class SetTy_ > 
void Shader::setParameter(parameter_handle handle, const VTy_& value, const VSetTy_ *valueset, int nvals, SetFnTy_ fn, SetTy_& valSet)
{
	if(handle != NULL)
	{
		SetTy_::iterator itr = valSet.find(handle);
		if(itr == valSet.end())
		{
			fn(handle, nvals, valueset);
			REMOVE_IN_RELEASE(checkForCgError("setting param", context()));
			valSet[handle] = value;
		}
		else if(itr->second != value)
		{
			fn(handle, nvals, valueset);
			REMOVE_IN_RELEASE(checkForCgError("setting param", context()));
			itr->second = value;
		}
	}
}

inline bool Shader::loadTechniques()
{
	// find first/all valid techniques
	_technique = cgGetFirstTechnique(_handle);
	checkForCgError("loading techniques", context());
	while (_technique) 
	{
		parseParameters();
		//recompileTechnique();
		if(cgValidateTechnique(_technique) == CG_TRUE)
		{
			_revalidationRequired = false;
			return true;
			//_validTechniques.push_back(tech);
			//if(!allTechniques)
			//	break;
		}
		checkForCgError("loading techniques", context());
		_technique = cgGetNextTechnique(_technique);
		checkForCgError("loading techniques", context());
	}
	return false;
}

struct ApplyParameterVisitor : public boost::static_visitor<>
{
private:
	Shader::parameter_handle handle;

public:
	ApplyParameterVisitor(Shader::parameter_handle handle_) : handle(handle_) {}
	// use boost preprocessor to create list of types, attach them to other required setting parameters, and generate the visitor functions
	void operator()( int value ) const
	{ 
		cgSetParameterValueir( handle, 1, &value ) ; 
	}
	void operator()( float value ) const
	{ 
		cgSetParameterValuefr( handle, 1, &value ) ; 
	}
	void operator()( const math::Vector2f& value ) const
	{ 
		cgSetParameterValuefr( handle, 2, reinterpret_cast<const float *>(&value) ) ; 
	}
	void operator()( const math::Vector3f& value ) const
	{ 
		cgSetParameterValuefr( handle, 3, reinterpret_cast<const float *>(&value) ) ; 
	}
	void operator()( const math::Vector4f& value ) const
	{ 
		cgSetParameterValuefr( handle, 4, reinterpret_cast<const float *>(&value) ) ; 
	}
	void operator()( const math::Matrix3f& value ) const
	{ 
		cgSetParameterValuefr( handle, 9, reinterpret_cast<const float *>(&value) ) ; 
	}
	void operator()( const math::Matrix4f& value ) const
	{ 
		cgSetParameterValuefr( handle, 16, reinterpret_cast<const float *>(&value) ) ; 
	}
	void operator()( const std::vector<float>& value ) const
	{ 
		cgSetParameterValuefr( handle, value.size(), reinterpret_cast<const float *>(&value[0]) ) ; 
	}
	void operator()( const std::vector<math::Vector2f>& value ) const
	{ 
		cgSetParameterValuefr( handle, 2 * value.size(), reinterpret_cast<const float *>(&value[0]) ) ; 
	}
	void operator()( const std::vector<math::Vector3f>& value ) const
	{ 
		cgSetParameterValuefr( handle, 3 * value.size(), reinterpret_cast<const float *>(&value[0]) ) ; 
	}
	void operator()( const std::vector<math::Vector4f>& value ) const
	{ 
		cgSetParameterValuefr( handle, 4 * value.size(), reinterpret_cast<const float *>(&value[0]) ) ; 
	}
	void operator()( const std::vector<math::Matrix3f>& value ) const
	{ 
		cgSetParameterValuefr( handle, 9 * value.size(), reinterpret_cast<const float *>(&value) ) ; 
	}
	void operator()( const std::vector<math::Matrix4f>& value ) const
	{ 
		cgSetParameterValuefr( handle, 16 * value.size(), reinterpret_cast<const float *>(&value) ) ; 
	}
	void operator()( glbase::Texture::ptr value ) const
	{ 
		cgGLSetTextureParameter( handle, value->handle() );
	}
};	

inline void Shader::parseParameters()
{
	// find all parameters
	CGparameter param = cgGetFirstEffectParameter(_handle);
	checkForCgError("parsing parameters", context());
	while(param)
	{
		CGtype cgType = cgGetParameterType(param);
		std::string name(cgGetParameterName(param));
		if(cgType != CG_UNKNOWN_TYPE)
		{
			// check if this is an unsized array parameter and we should size it,
			// do this first so we can set size of literal parameters before an attempt is made to set their value
			ParameterNameSizeMap::const_iterator aItr = _arraySizes.find(name);
			if(aItr != _arraySizes.end())
			{
				if(cgType != CG_ARRAY)
					throw std::runtime_error(std::string("Trying to set size of non array parameter:\n") + name);
				applyArraySize(param, aItr->second);
			}

			// check if we should make this parameter a literal, and set its value
			ParameterNameValueMap::const_iterator lItr = _literalParameters.find(name);
			if(lItr != _literalParameters.end())
				applyLiteralParameter(param, lItr->second);

			_parameters[name] = ParameterDesc(cgType, param, name);
		}
		else
		{
			throw std::runtime_error(std::string("Unknown parameter:\n ") + name);
		}

		checkForCgError("parsing parameters", context());
		param = cgGetNextParameter(param);
		checkForCgError("parsing parameters", context());
	}

	//// get automatic (inverse) matrix parameters
	//_autoMatrixMap.clear();
	////////////////////////////////////////////////////////////////////////////
	//// model view projection
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::MODEL_VIEW_PROJECTION_MAT.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
	////cgGLSetStateMatrixParameter(param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::MODEL_VIEW_PROJECTION_MAT_I.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_INVERSE);
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::MODEL_VIEW_PROJECTION_MAT_IT.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_INVERSE_TRANSPOSE);
	////cgGLSetStateMatrixParameter(param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_INVERSE);

	////////////////////////////////////////////////////////////////////////////
	//// model view
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::MODEL_VIEW_MAT.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY);
	////cgGLSetStateMatrixParameter(param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY);
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::MODEL_VIEW_MAT_I.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
	////cgGLSetStateMatrixParameter(param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::MODEL_VIEW_MAT_IT.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE_TRANSPOSE);

	////////////////////////////////////////////////////////////////////////////
	//// projection 
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::PROJECTION_MAT.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
	////cgGLSetStateMatrixParameter(param, CG_GL_PROJECTION_MATRIX , CG_GL_MATRIX_IDENTITY);
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::PROJECTION_MAT_I.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_PROJECTION_MATRIX, CG_GL_MATRIX_INVERSE);
	////cgGLSetStateMatrixParameter(param, CG_GL_PROJECTION_MATRIX , CG_GL_MATRIX_INVERSE);
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::PROJECTION_MAT_IT.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_PROJECTION_MATRIX, CG_GL_MATRIX_INVERSE_TRANSPOSE);

	////////////////////////////////////////////////////////////////////////////
	//// texture
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::TEXTURE_MAT.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_TEXTURE_MATRIX, CG_GL_MATRIX_IDENTITY);
	////cgGLSetStateMatrixParameter(param, CG_GL_TEXTURE_MATRIX , CG_GL_MATRIX_IDENTITY);
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::TEXTURE_MAT_I.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_TEXTURE_MATRIX, CG_GL_MATRIX_INVERSE);
	////cgGLSetStateMatrixParameter(param, CG_GL_TEXTURE_MATRIX , CG_GL_MATRIX_INVERSE);
	//param =	cgGetEffectParameterBySemantic(handle(), ShaderSymantics::TEXTURE_MAT_IT.c_str());
	//if(param != NULL)
	//	_autoMatrixMap[param] = AutoMatrix(CG_GL_TEXTURE_MATRIX, CG_GL_MATRIX_INVERSE_TRANSPOSE);

	//////////////////////////////////////////////////////////////////////////
	_semanticMap.clear();
	// check parameters for annotations and store them in a map
	for(ConstParameterMapIterator pItr = beginParameterMap(); pItr != endParameterMap(); pItr++)
	{
		std::string symantic(cgGetParameterSemantic(pItr->second.handle));
		if(!symantic.empty())
		{
			_semanticMap.insert(SemanticMapType::value_type(symantic, pItr->first));
		}
	}
}
inline void Shader::applyLiteralParameter( CGparameter param, const ParameterVariantType& literalParam )
{
	boost::apply_visitor(ApplyParameterVisitor(param), literalParam);
	checkForCgError("setting literal parameter value", context());
	cgSetParameterVariability(param, CG_LITERAL);
	checkForCgError("setting parameter variability", context());
	_revalidationRequired = true;
}

inline void Shader::applyArraySize( CGparameter param, unsigned int size )
{
	cgSetArraySize(param, size);
	checkForCgError("setting array parameter size", context());
	_revalidationRequired = true;
}
