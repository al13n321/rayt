#pragma once

#include "common.h"

namespace rayt {

class TestStopwatch {
public:
	TestStopwatch();
	~TestStopwatch();

	double TimeSinceRestart() const;
	double Restart();

	void ReportAndRestart(const char *task_name);
	void ReportIfGreaterAndRestart(const char *task_name, double min_time);
private:
	double clocks_per_sec;
	long long start_time;
};

}
