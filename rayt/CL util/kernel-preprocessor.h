#pragma once

#include "common.h"
#include <set>
#include <string>

namespace rayt {

    class KernelPreprocessor {
    public:
		KernelPreprocessor();
		~KernelPreprocessor();

		std::string LoadAndPreprocess(std::string working_dir, std::string file_name);
    private:
		std::string LoadAndPreprocess(std::string working_dir, std::string file_name, std::set<std::string> included_from);

		DISALLOW_COPY_AND_ASSIGN(KernelPreprocessor);
    };
    
}
