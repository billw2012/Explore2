// Stub implementations of ScriptLibrary for builds without boost-python.
// Python scripting is not used by the Explore2 game at runtime.
#include "script.h"

namespace vice {

Script::Script() : _isLoaded(false), _isEvaluated(false) {}

Script::Script(const boost::filesystem::path& file)
    : _file(file), _isLoaded(false), _isEvaluated(false) {}

bool Script::load() { return false; }

void ScriptLibrary::register_script(boost::filesystem::path, const boost::filesystem::path&) {}

Script& ScriptLibrary::get_script(boost::filesystem::path path, const boost::filesystem::path& context)
{
    static Script dummy;
    return dummy;
}

bool ScriptLibrary::execute_script(boost::filesystem::path, const boost::filesystem::path&, bool)
{
    return false;
}

std::unordered_map<std::string, Script> ScriptLibrary::_scripts;

}
