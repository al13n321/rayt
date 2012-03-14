#include "kernel-preprocessor.h"
#include <vector>
#include "string-util.h"
using namespace std;

namespace rayt {

	KernelPreprocessor::KernelPreprocessor() {}

	KernelPreprocessor::~KernelPreprocessor() {}

	std::string KernelPreprocessor::LoadAndPreprocess(std::string working_dir, std::string file_name) {
		if (working_dir.length() > 0) {
			char last = working_dir[working_dir.length() - 1];
			if (last != '/' && last != '\\')
				working_dir += "/";
		}
		return LoadAndPreprocess(working_dir, file_name, set<string>());
	}

	std::string KernelPreprocessor::LoadAndPreprocess(std::string working_dir, std::string file_name, std::set<std::string> included_from) {
		if (included_from.count(file_name))
			crash(string("include loop in ") + working_dir + file_name);
		included_from.insert(file_name);
		string text;
		if (!ReadFile(working_dir + file_name, text))
			crash(string("failed to read ") + working_dir + file_name);
		vector<string> lines = Tokenize(text, "\n\r");
		text = "";
		for (int i = 0; i < (int)lines.size(); ++i) {
			string &line = lines[i];
			string directive = "##include";
			if (line.length() > directive.length() && line.substr(0, directive.length()) == directive) {
				size_t pos1 = line.find_first_of('"');
				if (pos1 == string::npos)
					crash(string("invalid syntax: ") + line);
				size_t pos2 = line.find_first_of('"', pos1 + 1);
				if (pos2 == string::npos)
					crash(string("invalid syntax: ") + line);
				string included = line.substr(pos1 + 1, pos2 - pos1 - 1);
				line = LoadAndPreprocess(working_dir, included, included_from);
			}
			text += line;
			text += '\n';
		}
		return text;
	}

}
