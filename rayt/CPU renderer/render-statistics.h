#pragma once

#include "statistical-value.h"
#include <string>

namespace rayt {

struct RayStatistics {
	int traverse_steps_count;
	int traverse_visited_nodes_count;
};

class RenderStatistics {
public:
	RenderStatistics();
	~RenderStatistics();

	void Reset();
	void NewFrame();
	void ReportRay(const RayStatistics &p);

	std::string FormatShortReport();
private:
	StatisticalValue traverse_steps_count_;
	StatisticalValue traverse_visited_nodes_count_;
};

}
