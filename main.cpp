#include "sj_conv.hpp"
#include "utils/conv_baseline.hpp"

#include <unistd.h>
#include <iostream>


#define IN_W 32
#define IN_H 32

#define FILTER_WIDTH 3

#define OUT_W 32
#define OUT_H 32

#define F_IN 128
#define F_OUT 128


void check_result(float* output_base, float* output) {

	int output_img_size = OUT_W * OUT_H;
	int output_size = F_OUT * output_img_size;

	int error_num = 0;
	for (int i = 0; i < output_size; ++i) {

		if (abs(output_base[i] - output[i]) > 1e-3) {

		  int out_c = i / output_img_size;
		  int out_h = (i - out_c * output_img_size) / OUT_W;
		  int out_w = i - out_c * output_img_size - out_h * OUT_W;

		  std::cout << "Failed test at (" << out_c << ", " << out_h << ", " << out_w << ") with " << output_base[i] << " and " << output[i] << std::endl;
		  error_num++;
		  if (error_num > 10) {
		    break;
		  }
		}
	}

	std::cout << "Everything seems to work fine!" << std::endl;
}


int main(int argc, char ** argv) {



	int input_img_size = IN_W * IN_H;
	int output_img_size = OUT_W * OUT_H;


	int input_size = F_IN * input_img_size;
	int output_size = F_OUT * output_img_size;
	int filter_size = FILTER_WIDTH * FILTER_WIDTH * F_IN * F_OUT;


	float* input = new float[input_size];
	float* output = new float[output_size];
	float* filter = new float[filter_size];


	for (int i = 0; i < input_size; ++i) {
		input[i]  = ((float) rand() / (RAND_MAX)) - .5;
	}

	for (int i = 0; i < filter_size; ++i) {
		filter[i] = ((float) rand() / (RAND_MAX)) - .5;
	}


	float execution_time = sj_convolution(
		input, output, filter,
		F_IN, F_OUT,
		IN_H, IN_W, 
		OUT_H, OUT_W, 
		FILTER_WIDTH, FILTER_WIDTH
	);


	float* output_base = new float[output_size];
	baseline_conv(
		input, filter,
		F_IN, F_OUT,
		IN_W, IN_H,
		OUT_W, OUT_H,
		FILTER_WIDTH, FILTER_WIDTH,
		1, 1,
		1, 1,
		1, 1,
		false, output_base
	);

	check_result(output_base, output);


	std::cout << "The execution_time is " << execution_time << std::endl;

	delete[] input;
	delete[] filter;
	delete[] output;
	delete[] output_base;

	return 0;

}




