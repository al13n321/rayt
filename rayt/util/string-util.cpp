#include "string-util.h"

namespace rayt {

std::string WstringToString(const std::wstring &wstr) {
    char *buf = new char[wstr.length() * 2 + 1];
    buf[wcstombs(buf, wstr.c_str(), wstr.length())] = '\0';
    std::string res = buf;
    delete[] buf;
    return res;
}

std::wstring StringToWstring(const std::string &str) {
    wchar_t *buf = new wchar_t[str.length() + 1];
    buf[mbstowcs(buf, str.c_str(), str.length())] = '\0';
    std::wstring res = buf;
    delete[] buf;
    return res;
}

}