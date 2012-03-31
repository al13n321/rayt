#pragma once

#include "cl-context.h"
#include "array.h"
#include "profiler.h"

#ifdef PROFILING_ENABLED

#define PROFILE_CL_EVENT(event, name) ((rayt::CLEvent&)event).BeginAddToProfiler(&rayt::Profiler::default_profiler(), name)
#define PROFILE_CL_EVENTS_FINISH() rayt::CLEvent::FinishAddingToProfiler()

#else // PROFILING_ENABLED

#define PROFILE_CL_EVENT(event, name)
#define PROFILE_CL_EVENTS_FINISH()

#endif // PROFILING_ENABLED

namespace rayt {

    class CLEvent {
    public:
		CLEvent();
        CLEvent(cl_event event);
		CLEvent(const CLEvent &e);
        ~CLEvent();
        
		CLEvent& operator=(const CLEvent &e);

		void reset(cl_event e);

		// please don't release it;
		// can be null
        cl_event event() const;

		void WaitFor() const;

#ifdef PROFILING_ENABLED
		// To profile events, first BeginAddToProfiler them, then call FinishAddingToProfiler.
		// FinishAddingToProfiler waits for all enqueued events and dequeues them.
		void BeginAddToProfiler(Profiler *profiler, const std::string &timer_name);
		static void FinishAddingToProfiler();
#endif
	private:
        cl_event event_;
    };

	class CLEventList {
	public:
		CLEventList();
		CLEventList(const CLEventList &l);
		~CLEventList();

		CLEventList& operator=(const CLEventList &e);

		void Clear();
		void Add(const CLEvent &e);

		int size() const;

		void WaitFor() const;

		const cl_event* events() const;
	private:
		Array<cl_event> events_;
	};
    
}
