This repo is a demo of the technical report of

[HNMTP Conv: Optimize Convolution Algorithm for Single-Image Convolution Neural Network Inference on Mobile GPUs](https://arxiv.org/abs/1909.02765)
 
It achieves 14.6x speedup than the most popular im2col convolution algorithm, and 2.1x speedup than the fastest existing convolution algorithm (direct convolution) as far as we know.


Dependency: 
 
``
OpenCL, clBLAS, OpenBLAS(for result checking) 
``


How to use:
 
``
	mkdir build && cd build 
	cmake .. 
	make -j16  
``

This is just a prototype to illustrate the idea.

Code refactoring is on-going.
