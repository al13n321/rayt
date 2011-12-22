#include "test-stopwatch.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#include <iostream>
using namespace std;

namespace rayt {

static long long GetCurrentTime() {
#ifdef WIN32
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
#endif

#ifdef __APPLE__
    return static_cast<long long>(mach_absolute_time());
#endif
}
    
static double GetClocksPerSecond() {
#ifdef WIN32
    LARGE_INTEGER t;
	QueryPerformanceFrequency(&t);
	return static_cast<double>(t.QuadPart);
#endif
    
#ifdef __APPLE__
    mach_timebase_info_data_t timebase;
    kern_return_t error = mach_timebase_info(&timebase);
    if(error == 0)
        return 1e9 * (double) timebase.numer / (double) timebase.denom;
    else
        return 0;
#endif
}
    
TestStopwatch::TestStopwatch() {
	clocks_per_sec = GetClocksPerSecond();
	Restart();
}

TestStopwatch::~TestStopwatch() {}

double TestStopwatch::TimeSinceRestart() const {
	long long t = GetCurrentTime();
	return (t - start_time) / clocks_per_sec;
}

double TestStopwatch::Restart() {
	long long t = GetCurrentTime();
	double res = (t - start_time) / clocks_per_sec;
	start_time = t;
	return res;
}

void TestStopwatch::ReportAndRestart(const char *task_name) {
	ReportIfGreaterAndRestart(task_name, -1);
}

void TestStopwatch::ReportIfGreaterAndRestart(const char *task_name, double min_time) {
	long long t = GetCurrentTime();
	double r = (t - start_time) / clocks_per_sec;
	if (r > min_time)
		cout << task_name << " took " << r << " seconds" << endl;
	start_time = t;
}

}
