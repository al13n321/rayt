#pragma once

#include "common.h"
#include "statistical-value.h"
#include "stopwatch.h"
#include <map>
#include <stack>

#ifdef PROFILING_ENABLED

#define PROFILE_TIMER_START(name) rayt::Profiler::default_profiler().StartTimer(name)
#define PROFILE_TIMER_STOP() rayt::Profiler::default_profiler().StopLatestTimer()
#define PROFILE_TIMER_COMMIT(name) rayt::Profiler::default_profiler().CommitTimer(name)

#define PROFILE_VALUE_ADD(name, value) rayt::Profiler::default_profiler().IncreaseValue(name, value)
#define PROFILE_VALUE_COMMIT_SET(name, value) rayt::Profiler::default_profiler().CommitValue(name, value)
#define PROFILE_VALUE_COMMIT(name) rayt::Profiler::default_profiler().CommitValue(name)

#else // PROFILING_ENABLED

#define PROFILE_TIMER_START(name)
#define PROFILE_TIMER_STOP()
#define PROFILE_TIMER_COMMIT(name)

#define PROFILE_VALUE_ADD(name, value)
#define PROFILE_VALUE_COMMIT_SET(name, value)
#define PROFILE_VALUE_COMMIT(name)

#endif // PROFILING_ENABLED

namespace rayt {

	// Has some overhead (see many std::map's over there?). Not expected to be precise for time periods less than 1e-4 seconds.
	// Not thread safe, use only from main thread.

	class Profiler {
	public:
		static Profiler& default_profiler();

		Profiler();
		~Profiler();

		void StartTimer(const std::string &name);
		void StopLatestTimer();
		void CommitTimer(const std::string &name); // committing running timer is valid iff it's latest

		// practically equivalent to StartTimer and StopLatestTimer in "seconds" seconds
		void AddTimerValue(const std::string &name, double seconds);

		void IncreaseValue(const std::string &name, double add);
		void CommitValue(const std::string &name);
		void CommitValue(const std::string &name, double add);

		void Reset();

		std::string FormatTableReport();
	private:
		static Profiler default_profiler_;

		std::map<std::string, StatisticalValue> timer_statistics_;
		std::map<std::string, StatisticalValue> value_statistics_;
		std::stack<std::pair<std::string, Stopwatch> > running_timers_;
		std::map<std::string, double> active_timers_;
		std::map<std::string, double> active_values_;

		Stopwatch total_stopwatch_;
		double non_idle_time_;

		DISALLOW_COPY_AND_ASSIGN(Profiler);
	};

}
