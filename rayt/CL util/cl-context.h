#pragma once

#include "common.h"

namespace rayt {

    class CLContext {
    public:
        CLContext();
        ~CLContext();
        
        cl_context context();
        CGLShareGroupObj share_group();
        cl_device_id device_id();
        cl_command_queue queue();
        
        void WaitForAll(); // it's clFinish actually
    private:
        cl_context context_;
        cl_command_queue queue_;
        CGLShareGroupObj share_group_;
        cl_device_id device_id_;
        
        DISALLOW_COPY_AND_ASSIGN(CLContext);
    };
    
}
