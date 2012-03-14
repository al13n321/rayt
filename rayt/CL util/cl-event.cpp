#include "cl-event.h"
#include <cassert>

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

	const cl_event* CLEventList::events() const {
		return events_.begin();
	}
    
}
