#pragma once

#include "cl-context.h"
#include "array.h"

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

		const cl_event* events() const;
	private:
		Array<cl_event> events_;
	};
    
}
