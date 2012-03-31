#include "profiler.h"
#include <sstream>
#include <iomanip>
using namespace std;

namespace rayt {

	Profiler& Profiler::default_profiler() {
		static Profiler default_profiler_;

		return default_profiler_;
	}

	Profiler::Profiler() {}

	Profiler::~Profiler() {}

	void Profiler::StartTimer(const string &name) {
		if (!active_timers_.count(name))
			active_timers_[name] = 0;
		running_timers_.push(make_pair(name, Stopwatch()));
	}

	void Profiler::StopLatestTimer() {
		assert(!running_timers_.empty());
		const string &name = running_timers_.top().first;
		double t = running_timers_.top().second.TimeSinceRestart();
		active_timers_[name] += t;
		running_timers_.pop();
		if (running_timers_.empty())
			non_idle_time_ += t;
	}

	void Profiler::IncreaseValue(const std::string &name, double add) {
		active_values_[name] += add;
	}

	void Profiler::CommitValue(const std::string &name) {
		CommitValue(name, 0);
	}

	void Profiler::CommitValue(const std::string &name, double add) {
		value_statistics_[name].AddValue(active_values_[name] + add);
		active_values_.erase(name);
	}

	void Profiler::AddTimerValue(const std::string &name, double seconds) {
		active_timers_[name] += seconds;
	}

	void Profiler::CommitTimer(const string &name) {
		if (!active_timers_.count(name))
			return;

		if(!running_timers_.empty() && running_timers_.top().first == name)
			StopLatestTimer();
		timer_statistics_[name].AddValue(active_timers_[name]);
		active_timers_.erase(name);
	}

	void Profiler::Reset() {
		assert(running_timers_.empty());
		assert(active_timers_.empty());
		assert(active_values_.empty());

		timer_statistics_.clear();
		value_statistics_.clear();

		total_stopwatch_.Restart();
		non_idle_time_ = 0;
	}

	std::string Profiler::FormatTableReport() {
		assert(running_timers_.empty());
		assert(active_timers_.empty());
		assert(active_values_.empty());

		stringstream ss;

		double t = total_stopwatch_.TimeSinceRestart();
		value_statistics_["unmeasured time"].AddValue((t - non_idle_time_) / t);

		const int name_width = 20;

		ss << "timers:\n" << setw(name_width) << "" << StatisticalValue::GetTableReportHeader() << '\n';
		
		for (map<string, StatisticalValue>::const_iterator it = timer_statistics_.begin(); it != timer_statistics_.end(); ++it) {
			ss << setw(name_width) << it->first << "   " << it->second.GetTableReportRow() << '\n';
		}

		ss << "\nvalues:\n" << setw(name_width) << "" << StatisticalValue::GetTableReportHeader() << '\n';
		
		for (map<string, StatisticalValue>::const_iterator it = value_statistics_.begin(); it != value_statistics_.end(); ++it) {
			ss << setw(name_width) << it->first << "   " << it->second.GetTableReportRow() << '\n';
		}

		return ss.str();
	}

}
