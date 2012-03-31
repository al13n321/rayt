#include "stopwatch.h"

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

static inline double GetClocksPerSecond() {
	static double saved = -1;

	if (saved >= 0)
		return saved;

#ifdef WIN32
	LARGE_INTEGER t;
	QueryPerformanceFrequency(&t);
	saved = static_cast<double>(t.QuadPart);
#endif
    
#ifdef __APPLE__
    mach_timebase_info_data_t timebase;
    kern_return_t error = mach_timebase_info(&timebase);
    if(error == 0)
        saved = 1e9 * (double) timebase.numer / (double) timebase.denom;
    else
        saved = 0;
#endif

	return saved;
}
    
Stopwatch::Stopwatch() {
	Restart();
}

Stopwatch::~Stopwatch() {}

double Stopwatch::TimeSinceRestart() const {
	long long t = GetCurrentTime();
	return (t - start_time) / GetClocksPerSecond();
}

double Stopwatch::Restart() {
	long long t = GetCurrentTime();
	double res = (t - start_time) / GetClocksPerSecond();
	start_time = t;
	return res;
}

void Stopwatch::ReportAndRestart(const char *task_name) {
	ReportIfGreaterAndRestart(task_name, -1);
}

void Stopwatch::ReportIfGreaterAndRestart(const char *task_name, double min_time) {
	long long t = GetCurrentTime();
	double r = (t - start_time) / GetClocksPerSecond();
	if (r > min_time)
		cout << task_name << " took " << r << " seconds" << endl;
	start_time = t;
}

}
