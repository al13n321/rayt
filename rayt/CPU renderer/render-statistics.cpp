#include "render-statistics.h"
#include <sstream>
using namespace std;

namespace rayt {

RenderStatistics::RenderStatistics() {}

RenderStatistics::~RenderStatistics() {}

void RenderStatistics::Reset() {
	traverse_steps_count_.Reset();
	traverse_visited_nodes_count_.Reset();
}

void RenderStatistics::NewFrame() {}

void RenderStatistics::ReportRay(const RayStatistics &p) {
	traverse_steps_count_.AddValue(p.traverse_steps_count);
	traverse_visited_nodes_count_.AddValue(p.traverse_visited_nodes_count);
}

std::string RenderStatistics::FormatShortReport() {
	stringstream ss;
	ss << "Steps: (" << traverse_steps_count_.FormatAvgMax() << ") Visited Nodes: (" << traverse_visited_nodes_count_.FormatAvgMax() << ")";
	return ss.str();
}

}
