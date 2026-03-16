#ifndef xml_utils_h__
#define xml_utils_h__

#include <string>
#include <sstream>

namespace vice {;

template < class Ty_ >
Ty_ parse_attribute(const char* str, Ty_ defaultVal)
{
	if(str == NULL) return defaultVal;
	std::stringstream ss(str);
	Ty_ val;
	ss >> val;
	return val;
}

inline std::string parse_attribute(const char* str, std::string defaultVal)
{
	if(str == NULL) return defaultVal;
	return str;
}

template <typename Ty_>
Ty_ parse_val(const std::string& str)
{
	std::stringstream ss(str);
	Ty_ v;
	ss >> v;
	return v;
}


}

#endif // xml_utils_h__
