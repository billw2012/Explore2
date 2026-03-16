#ifndef script_h__
#define script_h__

#include <unordered_map>
#include <boost/filesystem/path.hpp>

namespace vice {;

struct Context;

/*
 *	A v8 script wrapper
 */

struct Script
{
	friend struct ScriptLibrary;
	Script();
	Script(const boost::filesystem::path& file);

	bool load();

	bool is_loaded() const { return _isLoaded; }
	bool is_evaluated() const { return _isEvaluated; }

private:
	boost::filesystem::path _file;
	bool _isLoaded;
	bool _isEvaluated;
	std::string _code;
};

/*
 *	A script manager, categorising scripts by path and ensuring 
 *	we don't compile the same one multiple times.
 */
struct ScriptLibrary
{
	static void		register_script(boost::filesystem::path path, const boost::filesystem::path& context = boost::filesystem::path());
	static Script&	get_script(boost::filesystem::path path, const boost::filesystem::path& context = boost::filesystem::path());
	static bool		execute_script(boost::filesystem::path path, const boost::filesystem::path& context = boost::filesystem::path(), bool force = false);

private:
	static std::unordered_map< std::string, Script > _scripts;
};

}

#endif // script_h__
