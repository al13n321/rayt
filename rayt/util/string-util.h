#pragma once
#include "common.h"
#include <string>
#include <vector>

namespace rayt {

    std::string WstringToString(const std::wstring &wstr);
    std::wstring StringToWstring(const std::string &str);
 
	bool ReadFile(std::string file, std::string &res);

	// doesn't remove empty tokens
	std::vector<std::string> Tokenize(std::string &s, std::string delims);
    
}
