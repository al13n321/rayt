#include "string-util.h"
#include <fstream>
using namespace std;

namespace rayt {

    string WstringToString(const wstring &wstr) {
        char *buf = new char[wstr.length() * 2 + 1];
        buf[wcstombs(buf, wstr.c_str(), wstr.length())] = '\0';
        string res = buf;
        delete[] buf;
        return res;
    }

    wstring StringToWstring(const string &str) {
        wchar_t *buf = new wchar_t[str.length() + 1];
        buf[mbstowcs(buf, str.c_str(), str.length())] = '\0';
        wstring res = buf;
        delete[] buf;
        return res;
    }
    
    bool ReadFile(string file, string &res) {
		res.clear();
        const int bufsize = 4096;
        char buf[bufsize];
        ifstream in(file.c_str());
        if(in.fail())
            return false;
        int c;
        do {
            in.read(buf, bufsize);
            c = (int) in.gcount();
            res.append(buf, c);
        } while (c == bufsize);
        return true;
    }
	
	vector<string> Tokenize(string &s, string delims) {
		vector<string> res;
		res.push_back("");
		for (int i = 0; i < (int)s.length(); ++i) {
			if (delims.find(s[i]) == string::npos) {
				res.back() += s[i];
			} else {
				res.push_back("");
			}
		}
		return res;
	}

}