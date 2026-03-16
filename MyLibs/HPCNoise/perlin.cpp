
#include "perlin.hpp"

#include <fstream>

namespace HPCNoise {;

namespace internal {;

struct ShaderCode
{
	ShaderCode()
	{
		std::string base = ""
#include "shader_base.inl"
			;
		std::string insertpt1("INSERT_POINT_ms_grad4_eval");
		std::string insertpt0("INSERT_POINT_perlin");
		std::string::size_type p0 = base.find(insertpt0);
		std::string::size_type p1 = base.find(insertpt1);
		parts.push_back(base.substr(0, p0));
		parts.push_back(base.substr(p0 + insertpt0.length(), p1 - p0 - insertpt0.length()));
		parts.push_back(base.substr(p1 + insertpt1.length()));
	}
	std::vector<std::string> parts;
};

const std::vector<std::string>& get_shader_base()
{
	static ShaderCode code;
	return code.parts;
}

};

void init()
{

}

};