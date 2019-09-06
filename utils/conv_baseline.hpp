#ifndef CONV_BASELINE_HPP_
#define CONV_BASELINE_HPP_

void baseline_conv(
	float* bottom, float* weight, 
	const int f_in, const int f_out,
    const int in_height, const int in_width, 
    const int out_height, const int out_width, 
    const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w, 
    const int stride_h, const int stride_w, 
    const int dilation_h, const int dilation_w,
	bool is_1x1_, float* result
);



#endif