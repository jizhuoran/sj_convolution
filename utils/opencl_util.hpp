#ifndef OPENCL_UTIL
#define OPENCL_UTIL

#include <iostream>
#include <CL/cl.h>

#define OPENCL_BUILD_CHECK(condition) \
  do { \
    cl_int error = condition; \
    if (error != CL_SUCCESS) { \
      char *buff_erro; \
      cl_int errcode; \
      size_t build_log_len; \
      errcode = clGetProgramBuildInfo(program, deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_len); \
      if (errcode) { \
        std::cout << "clGetProgramBuildInfo failed at line " << __LINE__; \
        exit(-1); \
      } \
      buff_erro = (char *)malloc(build_log_len); \
      if (!buff_erro) { \
          printf("malloc failed at line %d\n", __LINE__); \
          exit(-2); \
      } \
      errcode = clGetProgramBuildInfo(program, deviceID, CL_PROGRAM_BUILD_LOG, build_log_len, buff_erro, NULL); \
      if (errcode) { \
          std::cout << "clGetProgramBuildInfo failed at line " << __LINE__; \
          exit(-3); \
      } \
      std::cout << "Build log: " << buff_erro; \
      free(buff_erro); \
      std::cout << "clBuildProgram failed"; \
      exit(EXIT_FAILURE); \
    } \
  } while(0)


#define OPENCL_CHECK(condition) \
  do { \
    cl_int error = condition; \
    if(error != CL_SUCCESS) { \
      std::cout << "This is a error for OpenCL "<< error << " in " << __LINE__ << " in " << __FILE__;\
      exit(1); \
    } \
  } while (0)


inline size_t GET_BLOCKS(const size_t N, const size_t M) {
  return (N + M - 1) / M * M;
}


#endif