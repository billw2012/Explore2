
#include "stdafx.h"
#include "shadermanager.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <unordered_map>
#include <sstream>

#include <boost/scoped_array.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <boost/variant/apply_visitor.hpp>

#include "GLBase/SDLGl.hpp"

using namespace boost::filesystem;

namespace effect {;

std::string strip_leading_spaces(const std::string& line)
{
	// strip leading white space
	std::string::size_type firstNotWS = line.find_first_not_of(" \t");
	if(firstNotWS == std::string::npos)
		return std::string();
	return line.substr(firstNotWS);
}

std::string strip_trailing_spaces(const std::string& line)
{
	// strip leading white space
	std::string::size_type lastNotWS = line.find_last_not_of(" \t\n\r");
	if(lastNotWS == std::string::npos)
		return std::string();
	return line.substr(0, lastNotWS+1);
}

bool parse_include(std::string line, std::string& includePath)
{
	static std::string PRAGMA_STR("pragma");
	static std::string INCLUDE_STR("include");
	// strip white space
	line = strip_leading_spaces(line);
	if(line.empty()) return false;
	// check for '#'
	if(line[0] != '#')
		return false;
	// remove the '#'
	line = line.substr(1);
	line = strip_leading_spaces(line);
	if(line.empty()) return false;
	// check for pragma string
	if(line.substr(0, PRAGMA_STR.length()) != PRAGMA_STR)
		return false;
	// remove pragma string
	line = line.substr(PRAGMA_STR.length());
	line = strip_leading_spaces(line);
	if(line.empty()) return false;
	// check for include string
	if(line.substr(0, INCLUDE_STR.length()) != INCLUDE_STR)
		return false;
	// remove include string
	line = line.substr(INCLUDE_STR.length());
	// strip all leading and trailing white space
	line = strip_leading_spaces(line);
	line = strip_trailing_spaces(line);
	if(line.empty()) return false;
	// check for first and last quotes
	if(line[0] != '"' || line[line.length()-1] != '"')
		return false;
	includePath = line.substr(1, line.length() - 2);
	return true;
}

bool parse_once(std::string line)
{
	static std::string PRAGMA_STR("pragma");
	static std::string ONCE_STR("once");
	// strip white space
	line = strip_leading_spaces(line);
	if(line.empty()) return false;
	// check for '#'
	if(line[0] != '#')
		return false;
	// remove the '#'
	line = line.substr(1);
	line = strip_leading_spaces(line);
	if(line.empty()) return false;
	// check for pragma string
	if(line.substr(0, PRAGMA_STR.length()) != PRAGMA_STR)
		return false;
	// remove pragma string
	line = line.substr(PRAGMA_STR.length());
	line = strip_leading_spaces(line);
	if(line.empty()) return false;
	// check for include string
	if(line.substr(0, ONCE_STR.length()) != ONCE_STR)
		return false;

	// remove include string
	line = line.substr(ONCE_STR.length());
	// strip all leading and trailing white space
	line = strip_leading_spaces(line);
	if(!line.empty()) return false;

	return true;
}

path make_path_global(const path& apath, const path& currDir)
{
	if(apath.is_absolute())
		return apath;
	assert(apath.has_relative_path());
	return currDir / apath;
}

void load_and_preprocess_file(const path& filePath, path currDir, 
							  std::string& text, std::string& extensionStrings, std::set<path>& onceOnlyFiles)
{
	path fullPath = make_path_global(filePath, currDir);
	currDir = fullPath.parent_path();

	if(onceOnlyFiles.find(fullPath) != onceOnlyFiles.end())
		return ;

	// read file
	std::ifstream file(fullPath.string().c_str());
	if(file.fail())
		return ;

	// parse file into lines
	while(!file.eof())
	{
		std::string line;
		std::getline(file, line);
		std::string includeFilePath;
		if(parse_include(line, includeFilePath))
		{
			load_and_preprocess_file(includeFilePath, currDir, text, extensionStrings, onceOnlyFiles);
		}
		else if(parse_once(line))
		{
			onceOnlyFiles.insert(fullPath);
		}
		else if (line.find("#extension") != std::string::npos)
		{
			extensionStrings += line; extensionStrings += "\r\n";
		}
		else
		{
			text += line; text += "\r\n";
		}
	}
}

std::string _lastListing;

struct ShaderHandleType
{
	ShaderManager::ShaderHandle handle;
	ShaderManager::LoadFlags shaderType;

	ShaderHandleType(ShaderManager::ShaderHandle handle_ = 0, ShaderManager::LoadFlags shaderType_ = ShaderManager::NON)
		: handle(handle_), shaderType(shaderType_) {}
};

typedef std::unordered_map< std::string, ShaderHandleType > ShaderMap;
ShaderMap _shaders;

std::vector<std::string> _extensions;
std::unordered_map<std::string, ShaderManager::define_variant_type> _defines; 

struct ApplyDefineVisitor : public boost::static_visitor<std::string>
{
	std::string _name;
public:
	ApplyDefineVisitor(const std::string& name) : _name(name) {}
	std::string operator()( int value ) const
	{ 
		return "const int " + _name + " = " + std::to_string(value) + ";";
	}
	std::string operator()( float value ) const
	{ 
		return "const float " + _name + " = " + std::to_string(value) + ";";
	}
	std::string operator()( const math::Vector2f& value ) const
	{ 
		return "const vec2 " + _name + " = vec2(" + std::to_string(value.x) + ", " + std::to_string(value.y) + ");";
	}
	std::string operator()( const math::Vector3f& value ) const
	{ 
		return "const vec3 " + _name + " = vec2(" + std::to_string(value.x) + ", " + std::to_string(value.y) + ", " + std::to_string(value.z) + ");";
	}
	std::string operator()( const math::Vector4f& value ) const
	{ 
		return "const vec4 " + _name + " = vec2(" + std::to_string(value.x) + ", " + std::to_string(value.y) + ", " + std::to_string(value.z) +  ", " + std::to_string(value.w) + ");";
	}
};	

ShaderManager::ShaderHandle load_shader(const path& fullPath, ShaderManager::LoadFlags flags) 
{

	static const std::string verString = "#version 440\n";
	std::string constantsString;
	for(auto itr = _defines.begin(); itr != _defines.end(); ++itr)
	{
		constantsString += boost::apply_visitor(ApplyDefineVisitor(itr->first), itr->second) + "\n";
	}
	std::string txt, extensionStrings;
	std::set<path> onceOnlyFiles;
	load_and_preprocess_file(fullPath, fullPath.parent_path(), txt, extensionStrings, onceOnlyFiles);

	ShaderManager::ShaderHandle handle = glCreateShader((flags & ShaderManager::LoadFlags::VERTEX)? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
	CHECK_OPENGL_ERRORS;

	std::string finalStr = verString + extensionStrings + constantsString + txt;
	const char* strPtr[] = { finalStr.c_str() };
	glShaderSource(handle, 1, strPtr, NULL);
	CHECK_OPENGL_ERRORS;
	glCompileShader(handle);

	GLint infoLogLength;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &infoLogLength);
	boost::scoped_array<GLchar> str(new GLchar[infoLogLength + 1]); 
	glGetShaderInfoLog(handle, infoLogLength, NULL, str.get());
	_lastListing = str.get();
	std::string::size_type currOffs = 0;
	currOffs = _lastListing.find("ERROR", currOffs);
	while(currOffs != std::string::npos)
	{
		_lastListing.insert(currOffs, "\r\n");
		currOffs += 5;
		currOffs = _lastListing.find("ERROR", currOffs);
	}

	GLint compileResult;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &compileResult);
	if(compileResult != GL_TRUE)
		return 0;
	return handle;
}

void ShaderManager::set_define(const std::string& name, const ShaderManager::define_variant_type& value)
{
	_defines[name] = value;
}

ShaderManager::ShaderHandle ShaderManager::get_shader(const std::string& fileName, LoadFlags flags)
{
	path fullPath(system_complete(fileName));

	if(flags & ShaderManager::FORCE_RELOAD || _shaders.find(fullPath.string()) == _shaders.end())
	{
		ShaderHandleType handleType = load_shader(fullPath, flags & SHADER_TYPE_MASK);
		_shaders[fileName] = handleType;
		return handleType.handle;
	}
	else
	{
		return _shaders[fileName].handle;
	}
}

const std::string& ShaderManager::get_last_listing()
{
	return _lastListing;
}

}