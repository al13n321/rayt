#include "cl-context.h"
#include <cassert>
using namespace std;
using namespace boost;

namespace rayt {

    CLContext::CLContext() {
#if defined(__APPLE__)
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
#elif defined(WIN32)
		HGLRC hrc = wglGetCurrentContext();
		HDC hdc = wglGetCurrentDC();

		if (!hrc || !hdc)
			crash("failed to get HDC or HGLRC");

		cl_platform_id platform_ids[16];
		size_t platform_count;

		int err = clGetPlatformIDs(sizeof(platform_ids) / sizeof(platform_ids[0]), platform_ids, &platform_count);
		if (err != CL_SUCCESS || !platform_count)
			crash("failed to get platforms");

		for (uint i = 0; i < platform_count; ++i) {
			char platform_name[100];
			char platform_version[100];
			char platform_vendor[100];
			char platform_extensions[1000];
            err = clGetPlatformInfo(platform_ids[i],
                                    CL_PLATFORM_NAME,
                                    sizeof(platform_name),
                                    platform_name,
                                    NULL);
			if (err != CL_SUCCESS)
				crash("failed to get platform name");
            err = clGetPlatformInfo(platform_ids[i],
                                    CL_PLATFORM_VERSION,
                                    sizeof(platform_version),
                                    platform_version,
                                    NULL);
			if (err != CL_SUCCESS)
				crash("failed to get platform version");
            err = clGetPlatformInfo(platform_ids[i],
                                    CL_PLATFORM_VENDOR,
                                    sizeof(platform_vendor),
                                    platform_vendor,
                                    NULL);
			if (err != CL_SUCCESS)
				crash("failed to get platform vendor");
            err = clGetPlatformInfo(platform_ids[i],
                                    CL_PLATFORM_EXTENSIONS,
                                    sizeof(platform_extensions),
                                    platform_extensions,
                                    NULL);
			if (err != CL_SUCCESS)
				crash("failed to get platform extensions");

			cout << "platform name: " << platform_name << endl;
			cout << "platform version: " << platform_version << endl;
			cout << "platform vendor: " << platform_vendor << endl;
			cout << "platform extensions: " << platform_extensions << endl;
			cout << endl;
		}

		cl_platform_id platform_id = platform_ids[0];

		typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetGLContextInfoKHR_fn)(
			const cl_context_properties *properties,
			cl_gl_context_info param_name,
			size_t param_value_size,
			void *param_value,
			size_t *param_value_size_ret);

		clGetGLContextInfoKHR_fn clGetGLContextInfoKHR;
		clGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn) clGetExtensionFunctionAddress("clGetGLContextInfoKHR");
		if (!clGetGLContextInfoKHR)
			crash("failed to query proc address for clGetGLContextInfoKHR");

		cl_context_properties properties[] = 
        {
			CL_CONTEXT_PLATFORM, (cl_context_properties) platform_id,
			CL_GL_CONTEXT_KHR,   (cl_context_properties) hrc,
			CL_WGL_HDC_KHR,      (cl_context_properties) hdc,
			0
        };

		size_t device_count;
		err = clGetGLContextInfoKHR(properties, 
		                            CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR,
		                            sizeof(device_id_), 
		                            &device_id_, 
		                            &device_count);
		if (err != CL_SUCCESS || !device_count)
			crash("failed to get CL device from GL context");

		char device_name[100];
		char device_version[100];
		cl_device_type device_type;
		cl_bool device_image_support;
		err = clGetDeviceInfo(device_id_, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
		if (err != CL_SUCCESS)
			crash("failed to get device name");
		err = clGetDeviceInfo(device_id_, CL_DEVICE_VERSION, sizeof(device_version), device_version, NULL);
		if (err != CL_SUCCESS)
			crash("failed to get device version");
		err = clGetDeviceInfo(device_id_, CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL);
		if (err != CL_SUCCESS)
			crash("failed to get device type");
		err = clGetDeviceInfo(device_id_, CL_DEVICE_IMAGE_SUPPORT, sizeof(device_image_support), &device_image_support, NULL);
		if (err != CL_SUCCESS)
			crash("failed to get device image support");

		cout << "device name: " << device_name << endl;
		cout << "device version: " << device_version << endl;
		cout << "device type: " << ((device_type & CL_DEVICE_TYPE_GPU) ? "GPU" : "not GPU") << endl;
		cout << "device image support: " << (device_image_support ? "yes" : "no") << endl;
		cout << endl;

		context_ = clCreateContext(properties,
		                           1,
		                           &device_id_,
		                           0,
		                           0,
		                           &err);
#endif

		
        
        queue_ = clCreateCommandQueue(context_, device_id_, 0, NULL);
        if (!queue_)
            crash("failed to create command queue");
    }
    
    CLContext::~CLContext() {
        clFinish(queue_);
        clReleaseCommandQueue(queue_);
        clReleaseContext(context_);
    }
    
    cl_context CLContext::context() {
        return context_;
    }
    
    cl_device_id CLContext::device_id() {
        return device_id_;
    }
    
    cl_command_queue CLContext::queue() {
        return queue_;
    }
    
    void CLContext::WaitForAll() {
        int err = clFinish(queue_);
        if (err != CL_SUCCESS)
            crash("failed to clFinish");
    }
    
}
