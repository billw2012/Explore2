#include <fstream>

#include "script.h"
#include "logging.h"
#define HAVE_HYPOT
#define HAVE_ROUND
#include <boost/python.hpp>
#include "component.h"

namespace vice {;

Script::Script() : _isLoaded(false), _isEvaluated(false)
{

}

Script::Script( const boost::filesystem::path& file ) : _file(file), _isLoaded(false), _isEvaluated(false)
{
	load();
}

bool Script::load()
{
	if(_isLoaded)
		return true;

	std::ifstream f(_file.string());
	if(f.fail())
	{
		Logging::global().log(Logging::Error) << "Could not open script file " << _file;
		return false;
	}
	std::stringstream buffer;
	buffer << f.rdbuf();
	_code = buffer.str();
	_isLoaded = true;
	//std::unique_ptr<v8::ScriptCompiler::Source> source(new v8::ScriptCompiler::Source(
	//	v8::String::NewFromUtf8(gIsolate, buffer.str().c_str())));
	//_script = v8::ScriptCompiler::CompileUnbound(gIsolate, source.get());
	//_file = file;

	return true;
}

//Script::bound_script Script::bind_to_context()
//{
//	return _script->BindToCurrentContext();
//}

void ScriptLibrary::register_script(boost::filesystem::path path, const boost::filesystem::path& context /*= boost::filesystem::path()*/)
{
	path = ComponentLibrary::resolve_path(path, context);
	auto fItr = _scripts.find(path.string());
	if(fItr == _scripts.end())
	{
		Script script(path);
		_scripts[path.string()] = script;
	}
}

Script& ScriptLibrary::get_script(boost::filesystem::path path, const boost::filesystem::path& context /*= boost::filesystem::path()*/)
{
	path = ComponentLibrary::resolve_path(path, context);
	auto fItr = _scripts.find(path.string());
	if(fItr != _scripts.end())
		return fItr->second;
	Script script(path);
	_scripts[path.string()] = script;
	return _scripts[path.string()];
}

bool ScriptLibrary::execute_script(boost::filesystem::path path, const boost::filesystem::path& context /*= boost::filesystem::path()*/, bool force)
{
	using namespace boost::python;

	path = ComponentLibrary::resolve_path(path, context);
	auto fItr = _scripts.find(path.string());
	if(fItr == _scripts.end())
	{
		Logging::global().log(Logging::Error) << "Could not find script file " << path;
		return false;
	}

	Script& script = fItr->second;
	if(!script.is_loaded())
	{
		script.load();
	}
	if(script.is_evaluated() && !force)
	{
		Logging::global().log(Logging::Message) << path << " is already evaluated.";
		return true;
	}

	// get or create the module, use filename by default
	object scriptModule(handle<>(borrowed(PyImport_AddModule(path.stem().string().c_str()))));
	object scriptNamespace = scriptModule.attr("__dict__");
	object builtin = import("__builtin__");
	dict scriptDict = extract<dict>(scriptNamespace);
	scriptDict["__builtins__"] = builtin;
	exec(script._code.c_str(), scriptNamespace, scriptNamespace);

	script._isEvaluated = true;

	return true;
}

std::unordered_map< std::string, Script > ScriptLibrary::_scripts;

}