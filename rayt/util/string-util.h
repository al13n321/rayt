#pragma once
#include "common.h"
#include <string>

namespace rayt {

std::string WstringToString(const std::wstring &wstr);
std::wstring StringToWstring(const std::string &str);
    
}
