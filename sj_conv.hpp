#ifndef SJ_CONV_H
#define SJ_CONV_H

#include "utils/opencl_util.hpp"


std::string conv_codegen(
  int local_dim1, int local_dim2, int local_dim3,
  int output_bucket_size,
  int trans_output,
  int filter_c, int filter_r,
  int f_in, int f_out,
  int in_w, int in_h);


float sj_convolution(
  float* data_in, float* data_out, float* filter,
  const int in_channels, const int out_channels,
  const int in_height, const int in_width, 
  const int out_height, const int out_width, 
  const int kernel_h, const int kernel_w);

#endif

