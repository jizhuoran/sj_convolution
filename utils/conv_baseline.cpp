#include <iostream>

#ifdef USE_OPEN_BLAS
#include <cblas.h>
#endif

#include "conv_baseline.hpp"

inline bool is_a_ge_zero_and_a_lt_b(int a, int b) {
  return static_cast<unsigned>(a) < static_cast<unsigned>(b);
}

void im2col(float* data_im, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w, const int stride_h,
    const int stride_w, const int dilation_h, const int dilation_w,
    float* data_col) {

  auto data_im_data = data_im;
  auto data_col_data = data_col;

  const int output_h = (height + 2 * pad_h -
    (dilation_h * (kernel_h - 1) + 1)) / stride_h + 1;
  const int output_w = (width + 2 * pad_w -
    (dilation_w * (kernel_w - 1) + 1)) / stride_w + 1;
  const int channel_size = height * width;
  for (int channel = channels; channel--; data_im_data += channel_size) {
    for (int kernel_row = 0; kernel_row < kernel_h; kernel_row++) {
      for (int kernel_col = 0; kernel_col < kernel_w; kernel_col++) {
        int input_row = -pad_h + kernel_row * dilation_h;
        for (int output_rows = output_h; output_rows; output_rows--) {
          if (!is_a_ge_zero_and_a_lt_b(input_row, height)) {
            for (int output_cols = output_w; output_cols; output_cols--) {
              *(data_col_data++) = 0;
            }
          } else {
            int input_col = -pad_w + kernel_col * dilation_w;
            for (int output_col = output_w; output_col; output_col--) {
              if (is_a_ge_zero_and_a_lt_b(input_col, width)) {
                *(data_col_data++) = data_im_data[input_row * width + input_col];
              } else {
                *(data_col_data++) = 0;
              }
              input_col += stride_w;
            }
          }
          input_row += stride_h;
        }
      }
    }
  }

}


void baseline_conv(
  float* bottom, float* weight, 
  const int f_in, const int f_out,
    const int in_height, const int in_width, 
    const int out_height, const int out_width, 
    const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w, 
    const int stride_h, const int stride_w, 
    const int dilation_h, const int dilation_w,
    bool is_1x1_, float* output
) {
	

  int output_img_size = out_height * out_width;

  float* col_buff;
	
	if (!is_1x1_) {
		col_buff = new float[f_in * kernel_h * kernel_w * out_width * out_height];
    for (int i = 0; i < f_in * kernel_h * kernel_w * out_width * out_height; ++i) {
      col_buff[i] = .0;
    }
		im2col(bottom, f_in,
			in_height, in_width, 
			kernel_h, kernel_w,
			pad_h, pad_w, 
			stride_h, stride_w, 
			dilation_h, dilation_w,
			col_buff
    );
	} else {
		col_buff = bottom;
	}


	int M = f_out, N = out_width * out_height, K = f_in * kernel_h * kernel_w;

  
  
#ifdef USE_OPEN_BLAS
	int lda = K, ldb = N;
  cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, M, N, K, 1., weight, lda, col_buff, ldb, .0, output, N);
#else
  for (int m = 0; m < M; m++) {
    for (int n = 0; n < N; n++) {
      output[m * N + n] = 0;
      for (int k = 0; k < K; k++) {
         output[m * N + n] += weight[m * K + k] * col_buff[k * N + n];
      }
      
    }
  }
#endif
  	
  
  if (!is_1x1_) {
    delete[] col_buff;
  }
      

}