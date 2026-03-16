// Effect.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "effect.h"
#include "tinyxml.h"

#include "Misc/filefuncs.hpp"

#include "glbase/sdlgl.hpp"

#include <boost/scoped_array.hpp>
#include <iostream>
#include <unordered_set>

#include <boost/filesystem.hpp>

#ifdef WIN32
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#endif

// #if defined(_DEBUG)
// #define CHECK_GL_ERROR //CHECK_OPENGL_ERRORS;
// #else
// #define CHECK_GL_ERROR
// #endif

//void check_gl_error()
//{
//	GLenum err = glGetError();
//	CHECK_OPENGL_ERRORS;
//}

namespace effect{;

size_t Effect::_activeTexturesCount = 0;
std::vector<Effect::BoundTexture> Effect::_activeTextures;
Effect::ProgramHandle Effect::_boundShader = 0;
GLState::GLStateSet Effect::_currentState;

std::unordered_set<Effect*> _allEffects;

Effect::Effect() : _program(NULL), _shadowProgram(NULL), _boundStateType(EffectMode::Render) 
{
	_allEffects.insert(this);
}

Effect::~Effect()
{
	_allEffects.erase(this);
}

bool Effect::load_program_node(TiXmlElement* node, std::string& fileName,
					   StringStringSetMap& paramMap, const std::string& cwd)
{
	TiXmlElement* fileNode = node->FirstChildElement("file");
	if(fileNode == NULL) 
	{
		_lastError = "Missing file node in program node";
		return false;
	}
	const char* szFileName = fileNode->Attribute("name");
	if(NULL == szFileName)
	{
		_lastError = "Missing name attribute in program->file node";
		return false;
	}
	fileName = misc::get_full_path(szFileName, cwd);

	TiXmlElement* paramMapNode = node->FirstChildElement("map_param");
	while(paramMapNode)
	{
		const char* szSource = paramMapNode->Attribute("source");
		if(NULL == szSource)
		{
			_lastError = "Missing source attribute in program->map_param node";
			return false;
		}
		const char* szTarget = paramMapNode->Attribute("target");
		if(NULL == szTarget)
		{
			_lastError = "Missing target attribute in program->map_param node";
			return false;
		}
		paramMap[szSource].insert(szTarget);
		paramMapNode = paramMapNode->NextSiblingElement("map_param");
	}
	return true;
}

std::string get_program_log(GLuint handle)
{
	GLint infoLogLength;
	glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &infoLogLength);
 	boost::scoped_array<GLchar> str(new GLchar[infoLogLength + 1]); 
	glGetProgramInfoLog(handle, infoLogLength, NULL, str.get());
	return std::string(str.get());
}

void Effect::show_last_error_if_debug() const
{
#if defined(_DEBUG)
	std::cout << "Error creating shader " << _name << ": " << _lastError << std::endl;
#endif
}

void Effect::reload_all()
{
	for(auto itr = _allEffects.begin(); itr != _allEffects.end(); ++itr)
	{
		(*itr)->reload();
	}
}

bool Effect::reload()
{
	return load_internal(_file, true);
}

bool Effect::load(const boost::filesystem::path& file)
{
	return load_internal(file, false);
}

bool Effect::load_internal(const boost::filesystem::path& file, bool force)
{
	TiXmlDocument doc;

	boost::filesystem::path fullPath = boost::filesystem::absolute(file);

	if(!doc.LoadFile(fullPath.string().c_str()))
	{
		_lastError = "Could not open " + file.string();
		show_last_error_if_debug();
		return false;
	}

	TiXmlElement* root = doc.RootElement();

	if(root == NULL)
	{
		_lastError = "Could not get root element";
		show_last_error_if_debug();
		return false;
	}

	const char* szName = root->Attribute("name");
	if(NULL == szName)
	{
		_lastError = "Could not get name attribute from root element";
		show_last_error_if_debug();
		return false;
	}

	_file = fullPath;
	_name = szName;

	ShaderManager::ShaderHandle vp, fp, sfp, svp;
	StringStringSetMap vpParamMap, fpParamMap, shadowVPParamMap, shadowFPParamMap;
	if(load_shader(root, "vert_program", force? (ShaderManager::FORCE_RELOAD | ShaderManager::VERTEX) : ShaderManager::VERTEX,	fullPath.parent_path().string(), vp, vpParamMap) != ShaderLoadErrorType::NoError)
	{
		show_last_error_if_debug();
		return false;
	}
	if(load_shader(root, "frag_program", force? (ShaderManager::FORCE_RELOAD | ShaderManager::FRAGMENT) : ShaderManager::FRAGMENT, fullPath.parent_path().string(), fp, fpParamMap) == ShaderLoadErrorType::LoadError)
	{
		show_last_error_if_debug();
		return false;
	}
	_program = glCreateProgram();
	CHECK_OPENGL_ERRORS;
	glAttachShader(_program, vp);
	CHECK_OPENGL_ERRORS;
	if(fp != 0)
	{
		glAttachShader(_program, fp);
		CHECK_OPENGL_ERRORS;
	}
	glLinkProgram(_program);
	CHECK_OPENGL_ERRORS;
	GLint linkResult = GL_FALSE;
	glGetProgramiv(_program, GL_LINK_STATUS, &linkResult);
	if(linkResult == GL_FALSE)
	{
		_lastError = "Error linking program " + fullPath.string() + ": " + get_program_log(_program);
		show_last_error_if_debug();
		return false;
	}
	_paramMap.clear();
	map_parameters(_paramMap, vpParamMap, _program);
	map_parameters(_paramMap, fpParamMap, _program);

	if(load_shader(root, "shadow_vert_program", force? (ShaderManager::FORCE_RELOAD | ShaderManager::VERTEX) : ShaderManager::VERTEX, fullPath.parent_path().string(), svp, shadowVPParamMap) == ShaderLoadErrorType::LoadError)
	{
		show_last_error_if_debug();
		return false;
	}
	if(load_shader(root, "shadow_frag_program", force? (ShaderManager::FORCE_RELOAD | ShaderManager::FRAGMENT) : ShaderManager::FRAGMENT, fullPath.parent_path().string(), sfp, shadowFPParamMap) == ShaderLoadErrorType::LoadError)
	{
		show_last_error_if_debug();
		return false;
	}
	assert((svp != 0 && sfp != 0) || (svp == 0 && sfp == 0));

	if(svp != 0 && sfp != 0)
	{
		_shadowProgram = glCreateProgram();
		glAttachShader(_shadowProgram, svp);
		CHECK_OPENGL_ERRORS;
		glAttachShader(_shadowProgram, sfp);
		CHECK_OPENGL_ERRORS;
		glLinkProgram(_shadowProgram);
		CHECK_OPENGL_ERRORS;
		glGetProgramiv(_shadowProgram, GL_LINK_STATUS, &linkResult);
		if(linkResult == GL_FALSE)
		{
			_lastError = "Error linking shadow program " + fullPath.string() + ": " + get_program_log(_program);
			show_last_error_if_debug();
			return false;
		}
		_shadowParamMap.clear();
		map_parameters(_shadowParamMap, shadowVPParamMap, _shadowProgram);
		map_parameters(_shadowParamMap, shadowFPParamMap, _shadowProgram);
	}

	TiXmlElement* stateNode = root->FirstChildElement("state");
	if(stateNode != NULL)
		parse_state(stateNode, _stateSet);

	TiXmlElement* shadowStateNode = root->FirstChildElement("shadow_state");
	if(shadowStateNode != NULL)
		parse_state(shadowStateNode, _shadowStateSet);

	return true;
}

Effect::ShaderLoadErrorType::type Effect::load_shader(TiXmlElement* root, const char* elemName,
						 ShaderManager::LoadFlags shaderType, const std::string& fullDir, ShaderManager::ShaderHandle& shader,
						 StringStringSetMap& paramMap)
{
	shader = 0;
	TiXmlElement* node = root->FirstChildElement(elemName);
	if(node == NULL)
	{
		_lastError = "Could not get \"";
		_lastError += elemName;
		_lastError += "\" element";
		return ShaderLoadErrorType::NoNode;
	}

	std::string fileName;
	if(!load_program_node(node, fileName, paramMap, fullDir))
		return ShaderLoadErrorType::LoadError;
	shader = ShaderManager::get_shader(fileName, shaderType);
	if(shader == NULL)
	{
		_lastError = "Could not load shader \"" + fileName + "\", GLSL error:\n"
			+ ShaderManager::get_last_listing();
		return ShaderLoadErrorType::LoadError;
	}

	return ShaderLoadErrorType::NoError;
}

void execute_state_set(GLState::StateSetResetPair fn)
{
	fn.set();
	CHECK_OPENGL_ERRORS;
}

void execute_state_reset(GLState::StateSetResetPair fn)
{
	fn.reset();
	CHECK_OPENGL_ERRORS;
}

void Effect::apply_state(const GLState::GLStateUniqueSet& newState)
{
// 	reset_state();
// 	std::for_each(newState.begin(), newState.end(), execute_state_set);
// 	_currentState.insert(newState.begin(), newState.end());

	// When applying a new state set a few things must be done:

	// Determine what types of the current state are not in the new state set. These states will need to be 
	// reset. 
	GLState::GLStateUniqueSet unsetSet;
	for(auto sItr = _currentState.begin(); sItr != _currentState.end(); ++sItr)
	{
		auto fItr = newState.find(*sItr);
		if(fItr == newState.end())
		{
			unsetSet.insert(*sItr);
		}
	}
	// Determine what type value pairs of the new state are not in the current state set. These states will 
	// need to be set.
	GLState::GLStateSet setSet;
	for(auto sItr = newState.begin(); sItr != newState.end(); ++sItr)
	{
		auto fItr = _currentState.find(*sItr);
		if(fItr == _currentState.end())
		{
			setSet.insert(*sItr);
		}
	}
	// reset the states to be reset
	std::for_each(unsetSet.begin(), unsetSet.end(), execute_state_reset);
	for(auto sItr = unsetSet.begin(); sItr != unsetSet.end(); ++sItr)
		_currentState.erase(*sItr);

	// set the states to be set
	std::for_each(setSet.begin(), setSet.end(), execute_state_set);
	_currentState.insert(setSet.begin(), setSet.end());
	//for(GLStateUniqueSet::iterator sItr = unsetSet.begin(); sItr != unsetSet.end(); ++sItr)
	//for(GLStateSet::iterator sItr = setSet.begin(); sItr != setSet.end(); ++sItr)

	//for(size_t idx = 0; idx < state.size(); ++idx)
	//{
	//	const GLState::StateSetResetPair& newState = state[idx];

	//	if(_currentState.find(newState) == _currentState.end())
	//	{
	//		newState.set();
	//		_currentState.insert(newState);
	//	}
	//}

	//for(GLStateUniqueSet::iterator sItr = unsetSet.begin(); sItr != unsetSet.end(); ++sItr)
	//{
	//	sItr->reset();
	//	_currentState.erase(*sItr);
	//}
}

void Effect::reset_state()
{
	std::for_each(_currentState.begin(), _currentState.end(), execute_state_reset);
	_currentState.clear();
}

void Effect::set_state(EffectMode::type mode) const
{
	switch(mode)
	{
	case EffectMode::Render:
		apply_state(_stateSet);
		//std::for_each(_stateSet.begin(), _stateSet.end(), execute_state_set);
		break;
	case EffectMode::Shadow:
		apply_state(_shadowStateSet);
		//std::for_each(_shadowStateSet.begin(), _shadowStateSet.end(), execute_state_set);
		break;
	};
	_boundStateType = mode;
}

//void Effect::reset_state() const
//{
//	switch(_boundStateType)
//	{
//	case EffectMode::Render:
//		std::for_each(_stateSet.begin(), _stateSet.end(), execute_state_reset);
//		break;
//	case EffectMode::Shadow:
//		std::for_each(_shadowStateSet.begin(), _shadowStateSet.end(), execute_state_reset);
//		break;
//	};
//}

void Effect::bind(EffectMode::type mode) const
{
	ProgramHandle newProg = (mode == EffectMode::Render)? _program : _shadowProgram;
	if(_boundShader != newProg)
	{
		glUseProgram(newProg);
		CHECK_OPENGL_ERRORS;
		_boundShader = newProg;
	}
	//glActiveTexture(GL_TEXTURE0);
	//CHECK_OPENGL_ERRORS;
	_activeTexturesCount = 0;

	set_state(mode);
}

void Effect::unbind()
{
	glUseProgram(0);
	CHECK_OPENGL_ERRORS;
	_boundShader = 0;

	for(auto itr = _activeTextures.begin(); itr != _activeTextures.end(); ++itr)
		itr->unbind();
	glActiveTexture(GL_TEXTURE0);
	CHECK_OPENGL_ERRORS;
	_activeTexturesCount = 0;

	reset_state();
}

GLint Effect::set_texture(GLenum target, GLuint handle)
{
	assert(handle != 0);
	while(_activeTextures.size() <= _activeTexturesCount)
	{
		_activeTextures.push_back(BoundTexture(static_cast<GLuint>(_activeTextures.size())));
	}

	_activeTextures[_activeTexturesCount].set(target, handle);

	++_activeTexturesCount;

	return static_cast<GLint>(_activeTexturesCount) - 1;
}

void Effect::bind_texture(GLint index)
{
	_activeTextures[index].bind();
}

const std::string& Effect::get_last_error() const
{
	return _lastError;
}

void Effect::map_parameters(StringParamHandleSetMap& paramMap,
							const StringStringSetMap& namesMap, 
							ShaderManager::ShaderHandle prog)
{
	//GLint uCount;
	//glGetProgramiv(prog, GL_ACTIVE_UNIFORMS, &uCount);

	for(StringStringSetMap::const_iterator symItr = namesMap.begin(); 
		symItr != namesMap.end(); ++symItr)
	{
		for(StringSet::const_iterator nItr = symItr->second.begin(); 
			nItr != symItr->second.end(); ++nItr)
		{
			//const GLchar* name[] = {nItr->c_str()};
			//GLuint idx;
			//glGetUniformIndices(prog, 1, name, &idx);
			//if(idx == 0xffffffff)
			//{
			//	std::cout << "Warning: could not find shader variable handle for \"" <<
			//		*name << "\" in file \"" << _name << "\"" << std::endl;
			//}
			//else
			//{
			//	paramMap[symItr->first].insert(idx);
			//}
			paramMap[symItr->first].push_back(ParameterHandle(nItr->c_str()));
		}
	}
}

struct ApplyParameterVisitor : public boost::static_visitor<>
{
private:
	GLint _handle;
	GLboolean _matrixTranspose;
public:
	ApplyParameterVisitor(GLint handle, GLboolean matrixTranspose = GL_TRUE) 
		: _handle(handle), _matrixTranspose(matrixTranspose) {}

	void operator()( int value ) const
	{ 
		glUniform1i(_handle, value);
		CHECK_OPENGL_ERRORS;
	}
	void operator()( float value ) const
	{ 
		glUniform1f(_handle, value);
		CHECK_OPENGL_ERRORS;
	}
	void operator()( const math::Vector2f& value ) const
	{ 
		glUniform2fv(_handle, 1, &value.x);
		CHECK_OPENGL_ERRORS;
	}
	void operator()( const math::Vector3f& value ) const
	{ 
		glUniform3fv(_handle, 1, &value.x);
		CHECK_OPENGL_ERRORS;
	}
	void operator()( const math::Vector4f& value ) const
	{ 
		glUniform4fv(_handle, 1, &value.x);
		CHECK_OPENGL_ERRORS;
	}
	void operator()( const math::Matrix3f& value ) const
	{ 
		glUniformMatrix3fv(_handle, 1, _matrixTranspose, &value.m11);
		CHECK_OPENGL_ERRORS;
	}
	void operator()( const math::Matrix4f& value ) const
	{ 
		glUniformMatrix4fv(_handle, 1, _matrixTranspose, &value.m11);
		CHECK_OPENGL_ERRORS;
	}
	void operator()( const std::vector<int>& value ) const
	{ 
		if(!value.empty())
		{
			glUniform1iv(_handle, (GLsizei)value.size(), &(value[0]));
			CHECK_OPENGL_ERRORS;
		}
	}
	void operator()( const std::vector<float>& value ) const
	{ 
		if(!value.empty())
		{
			glUniform1fv(_handle, (GLsizei)value.size(), &(value[0]));
			CHECK_OPENGL_ERRORS;
		}
	}
	void operator()( const std::vector<math::Vector2f>& value ) const
	{ 
		if(!value.empty())
		{
			glUniform2fv(_handle, (GLsizei)value.size(), &(value[0].x));
			CHECK_OPENGL_ERRORS;
		}
	}
	void operator()( const std::vector<math::Vector3f>& value ) const
	{ 
		if(!value.empty())
		{
			glUniform3fv(_handle, (GLsizei)value.size(), &(value[0].x));
			CHECK_OPENGL_ERRORS;
		}
	}
	void operator()( const std::vector<math::Vector4f>& value ) const
	{ 
		if(!value.empty())
		{
			glUniform4fv(_handle, (GLsizei)value.size(), &(value[0].x));
			CHECK_OPENGL_ERRORS;
		}
	}
	void operator()( const std::vector<math::Matrix3f>& value ) const
	{ 
		if(!value.empty())
		{
			glUniformMatrix3fv(_handle, (GLsizei)value.size(), _matrixTranspose, &(value[0].m11));
			CHECK_OPENGL_ERRORS;
		}
	}
	void operator()( const std::vector<math::Matrix4f>& value ) const
	{ 
		if(!value.empty())
		{
			glUniformMatrix4fv(_handle, (GLsizei)value.size(), _matrixTranspose, &(value[0].m11));
			CHECK_OPENGL_ERRORS;
		}
	}
	void operator()( glbase::Texture::ptr value ) const
	{ 
		GLuint index = Effect::set_texture(value->target(), value->handle());
		glUniform1i(_handle, index);
		CHECK_OPENGL_ERRORS;
		Effect::bind_texture(index);
		CHECK_OPENGL_ERRORS;
	}
};	

void Effect::set_parameter(const std::string& param, const ParameterVariantType& val, EffectMode::type mode)
{
	StringParamHandleSetMap& paramMap = (mode == EffectMode::Render)? _paramMap : _shadowParamMap;
	ProgramHandle program = (mode == EffectMode::Render)? _program : _shadowProgram;

	StringParamHandleSetMap::iterator pItr = paramMap.find(param);
	if(pItr != paramMap.end())
	{
		ParamHandleSet& targetParams = pItr->second;
		for(ParamHandleSet::iterator cItr = targetParams.begin(); cItr != targetParams.end(); ++cItr)
		{
			ParameterHandle& handle = *cItr;
			if(handle.handle == -1)
			{
				GLint location = glGetUniformLocation(program, (const GLchar*)(handle.name.c_str()));
				if(location == -1)
					handle.handle = -2;
				else
					handle.handle = location;
			}
			if(handle.handle != -2)
				boost::apply_visitor(ApplyParameterVisitor(handle.handle), val);
		}
	}
}

bool Effect::validate_program() const
{
	glValidateProgram(_program);
	GLint linkResult = GL_FALSE;
	glGetProgramiv(_program, GL_VALIDATE_STATUS, &linkResult);
	if(linkResult == GL_FALSE)
	{
		_lastError = "Error validating program " + 
			_name + ": " + get_program_log(_program);
		return false;
	}
	return true;
}

void Effect::parse_state( TiXmlElement* stateNode, GLState::GLStateUniqueSet& stateVec )
{
	TiXmlAttribute* attrib = stateNode->FirstAttribute();
	while(attrib)
	{
		std::string name = attrib->Name();
		std::string value = attrib->Value();

		std::vector< std::string > valueVec;
		std::string::size_type lastPos = 0, commaPos = value.find_first_of(',');
		while(commaPos != std::string::npos)
		{
			valueVec.push_back(value.substr(lastPos, (commaPos - lastPos) /*- 1*/));
			lastPos = commaPos + 1;
			commaPos = value.find_first_of(',', lastPos);
		}
		valueVec.push_back(value.substr(lastPos));

		GLState::StateSetResetPair fn = StateTypes::get_instance()->parse_state(name, valueVec);
		stateVec.insert(fn);

		attrib = attrib->Next();
	}
}

}

