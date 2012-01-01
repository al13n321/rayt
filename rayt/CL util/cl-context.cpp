#include "cl-context.h"
#include <cassert>
//using namespace std;
//using namespace boost;

namespace rayt {

    CLContext::CLContext() {
        CGLContextObj kCGLContext = CGLGetCurrentContext();              
        CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
        
        cl_context_properties properties[] = { 
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, 
            (cl_context_properties)kCGLShareGroup, 0 
        };
        
        context_ = clCreateContext(properties, 0, 0, clLogMessagesToStdoutAPPLE, 0, 0);
        if (!context_)
            crash("failed to create context");
        
        uint device_count;
        cl_device_id device_ids[16];
        size_t returned_size;
        
        int err = clGetContextInfo(context_, CL_CONTEXT_DEVICES, sizeof(device_ids), device_ids, &returned_size);
        if (err != CL_SUCCESS)
            crash("failed to get devices");
        
        device_count = (int)returned_size / sizeof(cl_device_id);
        
        int i = 0;
        bool device_found = false;
        cl_device_type device_type;	
        for (i = 0; i < device_count; i++) 
        {
            clGetDeviceInfo(device_ids[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &device_type, NULL);
            if (device_type == CL_DEVICE_TYPE_GPU)
            {
                device_id_ = device_ids[i];
                device_found = true;
                break;
            }
        }
        
        if (!device_found)
            crash("no GPU device found");
        
        queue_ = clCreateCommandQueue(context_, device_id_, 0, NULL);
        if (!queue_)
            crash("failed to create command queue");
    }
    
    cl_context CLContext::context() {
        return context_;
    }
    
    CGLShareGroupObj CLContext::share_group() {
        return share_group_;
    }
    
    cl_device_id CLContext::device_id() {
        return device_id_;
    }
    
    cl_command_queue CLContext::queue() {
        return queue_;
    }
    
}
