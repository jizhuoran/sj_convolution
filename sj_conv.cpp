#include "utils/opencl_util.hpp"
#include "sj_conv.hpp"

#include <vector>

float sj_convolution(
  float* data_in, float* data_out, float* filter,
  const int in_channels, const int out_channels,
  const int in_height, const int in_width, 
  const int out_height, const int out_width, 
  const int kernel_h, const int kernel_w) {


	int input_size = in_channels * in_height * in_width;
    int output_size = out_channels * out_height * out_width;
    int filter_size = kernel_h * kernel_w * in_channels * out_channels;
  	int col_buffer_offset = in_channels *  kernel_h * kernel_w * out_height * out_width;

	const int TILE_SIZE_W = 4;
	const int TILE_SIZE_H = 2;
	const int TILE_SIZE_Z = 8;



	const int OUTPUT_BUCKET_SIZE = TILE_SIZE_W * TILE_SIZE_H;
	const int OUTPUT_BUCKET_NUM = out_channels / OUTPUT_BUCKET_SIZE;

	const bool trans_output = false;

	// Getting platform and device information
	cl_platform_id platformId = NULL;
	cl_device_id deviceID = NULL;
	cl_uint retNumDevices;
	cl_uint retNumPlatforms;
	cl_int ret;

	OPENCL_CHECK(clGetPlatformIDs(1, &platformId, &retNumPlatforms));
	OPENCL_CHECK(clGetDeviceIDs(platformId, CL_DEVICE_TYPE_DEFAULT, 1, &deviceID, &retNumDevices));

	// Creating context.
	cl_context context = clCreateContext(NULL, 1, &deviceID, NULL, NULL,  &ret);
	OPENCL_CHECK(ret);

	// Creating command queue
	cl_command_queue commandQueue = clCreateCommandQueue(context, deviceID, CL_QUEUE_PROFILING_ENABLE, &ret);
	OPENCL_CHECK(ret);


	// Memory buffers for each array
	cl_mem inputMemObj = clCreateBuffer(context, CL_MEM_READ_ONLY, input_size * sizeof(float), NULL, &ret);
	OPENCL_CHECK(ret);
	cl_mem outputMemObj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, output_size * sizeof(float), NULL, &ret);
	OPENCL_CHECK(ret);
	cl_mem filterMemObj = clCreateBuffer(context, CL_MEM_READ_ONLY, filter_size * sizeof(float), NULL, &ret);
	OPENCL_CHECK(ret);


	

	
	//transpose filter for coalescing read
	float* filter_trans = new float[filter_size];
	for (int i = 0; i < out_channels; ++i) {
		for (int j = 0; j < in_channels; ++j) {
			for (int k = 0; k < kernel_w * kernel_h; ++k) {
				filter_trans[((j * out_channels + i / OUTPUT_BUCKET_SIZE * OUTPUT_BUCKET_SIZE) * kernel_w * kernel_h) + k * OUTPUT_BUCKET_SIZE + i % OUTPUT_BUCKET_SIZE] = filter[((i * in_channels + j) * kernel_w * kernel_h) + k];
			}
		}
	}



	// Copy lists to memory buffers
	OPENCL_CHECK(clEnqueueWriteBuffer(commandQueue, inputMemObj, CL_TRUE, 0, input_size * sizeof(float), data_in, 0, NULL, NULL));
	OPENCL_CHECK(clEnqueueWriteBuffer(commandQueue, filterMemObj, CL_TRUE, 0,filter_size * sizeof(float), filter_trans, 0, NULL, NULL));
	float pattern = .0;	
	OPENCL_CHECK(clEnqueueFillBuffer(commandQueue, outputMemObj, &pattern, sizeof(float), 0, output_size * sizeof(float), 0, NULL, NULL));

	delete[] filter_trans;



	// Create program from kernel source
	std::string kernel_code = conv_codegen(
		TILE_SIZE_W, TILE_SIZE_H, TILE_SIZE_Z,
		OUTPUT_BUCKET_SIZE,
		trans_output,
		kernel_w, kernel_h,
		in_channels, out_channels,
		in_width, in_height
	);


	size_t kernel_size = kernel_code.size() + 1;	
  	const char* kernelSource = kernel_code.c_str();


	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&kernelSource, (const size_t *)&kernel_size, &ret);	
	OPENCL_CHECK(ret);



	// Build program
	OPENCL_BUILD_CHECK(clBuildProgram(program, 1, &deviceID, "-cl-std=CL1.2 -O3", NULL, NULL));


	
	cl_kernel kernel = clCreateKernel(program, "convolution", &ret);
  	OPENCL_CHECK(ret);


    OPENCL_CHECK(clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputMemObj));
    OPENCL_CHECK(clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputMemObj));
    OPENCL_CHECK(clSetKernelArg(kernel, 2, sizeof(cl_mem), &filterMemObj));

	cl_event event;

	std::vector<size_t> global_size{GET_BLOCKS(in_width, TILE_SIZE_W), GET_BLOCKS(in_height, TILE_SIZE_H), static_cast<size_t>(OUTPUT_BUCKET_NUM)};
	std::vector<size_t> local_size{static_cast<size_t>(TILE_SIZE_W), static_cast<size_t>(TILE_SIZE_H), static_cast<size_t>(TILE_SIZE_Z)};


  	OPENCL_CHECK(clEnqueueNDRangeKernel(
		commandQueue, 
		kernel, 
		global_size.size(), 
		NULL,
		global_size.data(), 
		local_size.data(), 
		NULL, 
		NULL, 
		&event)
  	);  



	clWaitForEvents(1, &event);
	clFinish(commandQueue);

	cl_ulong time_start;
	cl_ulong time_end;

	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
	clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

	float execute_time = (time_end-time_start) * 1.0 / 1000000;

	OPENCL_CHECK(clEnqueueReadBuffer(commandQueue, outputMemObj, CL_TRUE, 0, output_size * sizeof(float), data_out, 0, NULL, NULL));

	// Clean up, release memory.
	OPENCL_CHECK(clFlush(commandQueue));
	OPENCL_CHECK(clFinish(commandQueue));
	OPENCL_CHECK(clReleaseCommandQueue(commandQueue));
	OPENCL_CHECK(clReleaseProgram(program));
	OPENCL_CHECK(clReleaseMemObject(inputMemObj));
	OPENCL_CHECK(clReleaseMemObject(filterMemObj));
	OPENCL_CHECK(clReleaseMemObject(outputMemObj));
	OPENCL_CHECK(clReleaseContext(context));

	return execute_time;

}

