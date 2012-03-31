#include "cl-event.h"
#include <cassert>
#include <vector>
using namespace std;

namespace rayt {

	CLEvent::CLEvent() {
		event_ = NULL;
	}

    CLEvent::CLEvent(cl_event e) {
        event_ = e;
		if (e) {
			if (clRetainEvent(e) != CL_SUCCESS)
				crash("failed to retain event");
		}
    }
    
	CLEvent::CLEvent(const CLEvent &e) {
		event_ = e.event_;
		if (event_) {
			if (clRetainEvent(event_) != CL_SUCCESS)
				crash("failed to retain event");
		}
	}

    CLEvent::~CLEvent() {
		if (event_) {
			if (clReleaseEvent(event_) != CL_SUCCESS)
				crash("failed to release event");
		}
	}
    
	CLEvent& CLEvent::operator=(const CLEvent &e) {
		reset(e.event_);
		return *this;
	}

	void CLEvent::reset(cl_event e) {
		if (event_) {
			if (clReleaseEvent(e) != CL_SUCCESS)
				crash("failed to release event");
		}
		event_ = e;
		if (event_) {
			if (clRetainEvent(e) != CL_SUCCESS)
				crash("failed to retain event");
		}
	}

	cl_event CLEvent::event() const {
		return event_;
	}

	void CLEvent::WaitFor() const {
		if (clWaitForEvents(1, &event_) != CL_SUCCESS)
			crash("failed to wait for event");
	}

#ifdef PROFILING_ENABLED

	struct EnqueuedProfiledEvent {
		Profiler *profiler;
		std::string timer_name;
		CLEvent event;

		EnqueuedProfiledEvent(Profiler *profiler, const std::string &timer_name, const CLEvent &event) : profiler(profiler), timer_name(timer_name), event(event) {}
	};

	static vector<EnqueuedProfiledEvent> profiled_events_;

	void CLEvent::BeginAddToProfiler(Profiler *profiler, const std::string &timer_name) {
		profiled_events_.push_back(EnqueuedProfiledEvent(profiler, timer_name, *this));
	}

	void CLEvent::FinishAddingToProfiler() {
		for (int i = 0; i < (int) profiled_events_.size(); ++i) {
			EnqueuedProfiledEvent &ev = profiled_events_[i];
			ev.event.WaitFor();
			cl_ulong start, end;
			if (clGetEventProfilingInfo(ev.event.event(), CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start , NULL) != CL_SUCCESS ||
				clGetEventProfilingInfo(ev.event.event(), CL_PROFILING_COMMAND_END  , sizeof(cl_ulong), &end   , NULL) != CL_SUCCESS)
				crash("failed to get event profiling info");
			ev.profiler->AddTimerValue(ev.timer_name, (end - start) * 1e-9);
		}
		profiled_events_.clear();
	}

#endif

	CLEventList::CLEventList() {}

	CLEventList::CLEventList(const CLEventList &l) {
		events_ = l.events_;
		for (int i = 0; i < events_.size(); ++i)
			if (clRetainEvent(events_[i]) != CL_SUCCESS)
				crash("failed to retain event");
	}

	CLEventList::~CLEventList() {
		for (int i = 0; i < events_.size(); ++i)
			if (clReleaseEvent(events_[i]) != CL_SUCCESS)
				crash("failed to release event");
	}

	CLEventList& CLEventList::operator=(const CLEventList &l) {
		for (int i = 0; i < events_.size(); ++i)
			if (clReleaseEvent(events_[i]) != CL_SUCCESS)
				crash("failed to release event");
		events_ = l.events_;
		for (int i = 0; i < events_.size(); ++i)
			if (clRetainEvent(events_[i]) != CL_SUCCESS)
				crash("failed to retain event");
		return *this;
	}

	void CLEventList::Clear() {
		for (int i = 0; i < events_.size(); ++i)
			if (clReleaseEvent(events_[i]) != CL_SUCCESS)
				crash("failed to release event");
		events_.Resize(0);
	}

	void CLEventList::Add(const CLEvent &e) {
		events_.Add(e.event());
		if (clRetainEvent(e.event()) != CL_SUCCESS)
			crash("failed to retain event");
	}

	int CLEventList::size() const {
		return events_.size();
	}

	void CLEventList::WaitFor() const {
		if (clWaitForEvents(events_.size(), events_.begin()) != CL_SUCCESS)
			crash("failed to wait for events");
	}

	const cl_event* CLEventList::events() const {
		return events_.begin();
	}
    
}
