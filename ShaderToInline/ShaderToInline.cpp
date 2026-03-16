// ShaderToInline.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

std::string escape_string(const std::string& str)
{
	std::stringstream ss;
	static std::string fmtChars("\"\\"); 
	for(size_t idx = 0; idx < str.length(); ++idx)
	{
		char ch = str[idx];
		if(fmtChars.find(ch) != std::string::npos)
		{
			ss << '\\';
		}
		ss << ch;
	}
	return ss.str(); 
}

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc != 3)
	{
		std::cout << "Argument error" << std::endl;
		return 1;
	}

	std::ifstream shaderFile(argv[1]);
	std::ofstream inlineFile(argv[2]);

	std::string line;
	while (std::getline(shaderFile, line)) 
	{
		inlineFile << "\"" << escape_string(line) << "\\n\"" << std::endl;
	}

	return 0;
}

