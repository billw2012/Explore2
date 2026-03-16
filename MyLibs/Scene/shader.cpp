#include "shader.hpp"

namespace scene
{

boost::shared_ptr<ShaderManager> ShaderManager::_instance;

void checkForCgError(const std::string& status, CGcontext context) /*throw(compile_error, std::runtime_error)*/
{
	CGerror error = cgGetError();
	if (error != CG_NO_ERROR) 
	{
		const char *err = cgGetErrorString(error);
		if(error == CG_COMPILER_ERROR)
		{
			const char *compileErr = cgGetLastListing(context);
			std::string compileError(compileErr);
			throw compile_error(status, std::string(err) + ":\n" + compileError);
		}
		else
			throw std::runtime_error(std::string(err) + ":\n" + status);
	}
}

}