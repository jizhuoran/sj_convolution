#include "sj_conv.hpp"

#include <iostream>
#include <map>
#include <sstream>


void add_def(
  std::stringstream& ss,
  std::map<std::string, int>& params,
  const char* name, 
  int value) {
  
  ss << "#ifdef " << name << std::endl;
  ss << "#undef " << name << std::endl;
  ss << "#endif" << std::endl;
  ss << "#define " << name << " " << value << std::endl << std::endl;

  params.insert(std::pair<std::string, int>(name, value));

}


void conv_compute_unroll(std::stringstream& ss, const std::map<std::string, int>& params) {
	

    ss << "    dest = local_row * TILE_SIZE_W + local_col;" << std::endl;

    ss << "      #pragma unroll" << std::endl;
    ss << "      for (int k = 0; k < TILE_SIZE_Z; ++k) {" << std::endl;
    
    for (int r = 0; r < params.at("FILTER_R"); ++r) {
    	for (int c = 0; c < params.at("FILTER_C"); ++c) {
		    ss << "     {" << std::endl << std::endl;
		    ss << "      filter_reg_cache = filter[dest + (k + batch_off) * F_OUT * FILTER_SIZE + ((get_global_id(2) * FILTER_SIZE + " << r * params.at("FILTER_C") + c << ") * OUTPUT_BUCKET_SIZE)];" << std::endl;
		    ss << "      #pragma unroll" << std::endl;
		    ss << "      for (int j = 0; j < OUTPUT_BUCKET_SIZE; ++j) {" << std::endl;
		    ss << "        out_img_cache[j] += filter_reg_cache * in_img_cache[k][j/TILE_SIZE_W + " << r << "][(j%TILE_SIZE_W) + " << c << "];" << std::endl;
		    ss << "      }" << std::endl << std::endl;
		    ss << "     }" << std::endl << std::endl;
    	}
    }

    ss << "      }" << std::endl;

    
}



void load_img(std::stringstream& ss, const std::map<std::string, int>& params, bool aligned) {
	
	ss << "    #pragma unroll" << std::endl;
	ss << "    for (int i = 0; i < IMG_LOAD_TIME; ++i) {" << std::endl; 
	ss << "      dest = i * TILE_SIZE + local_row * TILE_SIZE_W + local_col;" << std::endl; 
	ss << "      destH = dest / FOOTPRINT_W;" << std::endl; 
	ss << "      destW = dest % FOOTPRINT_W;" << std::endl; 
	ss << "      srcH = get_group_id(1) * TILE_SIZE_H + destH - " << params.at("HALF_FILTER_SIZE") << ";" << std::endl; 
	ss << "      srcW = get_group_id(0) * TILE_SIZE_W + destW - " << params.at("HALF_FILTER_SIZE") << ";" << std::endl; 
	ss << "      src = (srcH * IN_W + srcW) + (get_local_id(2) + batch_off) * INPUT_SIZE;" << std::endl; 
	ss << "      " << std::endl; 
	ss << "      if (destH < FOOTPRINT_H) {" << std::endl; 
	ss << "        if (srcH >= 0 && srcH < IN_H && srcW >= 0 && srcW < IN_W)" << std::endl; 
	ss << "          in_img_cache[get_local_id(2)][destH][destW] = input[src];" << std::endl; 
	ss << "        else" << std::endl; 
	ss << "          in_img_cache[get_local_id(2)][destH][destW] = 0;" << std::endl; 
	ss << "      }" << std::endl; 
	ss << "    }" << std::endl; 
	ss << "    barrier(CLK_LOCAL_MEM_FENCE);" << std::endl << std::endl; 


}




void save_result(std::stringstream& ss, const std::map<std::string, int>& params) {

	if (params.at("TRANS_OUTPUT")) {
		ss << "  #pragma unroll" << std::endl;
		ss << "  for(int output_pos = 0; output_pos < OUTPUT_BUCKET_SIZE; ++output_pos) {" << std::endl;
		ss << "  	out_img_trans[get_local_id(2)][dest][output_pos] = out_img_cache[output_pos];" << std::endl;
		ss << "  }" << std::endl;

		ss << "  barrier(CLK_LOCAL_MEM_FENCE);" << std::endl;
		
		ss << "  #pragma unroll" << std::endl;
		ss << "  for(int output_pos = 0; output_pos < OUTPUT_BUCKET_SIZE; ++output_pos) {" << std::endl;
		ss << "  	output[(output_pos + OUTPUT_BUCKET_SIZE * get_global_id(2)) * INPUT_SIZE + get_global_id(1) * OUT_W + get_global_id(0)] = out_img_trans[get_local_id(2)][output_pos][dest];" << std::endl;
		ss << "  }" << std::endl;
	} else {
		ss << "  #pragma unroll" << std::endl;
		ss << "  for(int output_pos = 0; output_pos < OUTPUT_BUCKET_SIZE; ++output_pos) {" << std::endl;
		ss << "  	src = (get_group_id(1) * TILE_SIZE_H + (output_pos) / TILE_SIZE_W) * OUT_W + (get_group_id(0) * TILE_SIZE_W + (output_pos) % TILE_SIZE_W);" << std::endl;
		ss << "  	output[(dest + OUTPUT_BUCKET_SIZE * get_global_id(2)) * INPUT_SIZE + src] = out_img_cache[output_pos];" << std::endl;
		ss << "  }" << std::endl;	
	}
	
}



void conv_code_body(
	std::stringstream& ss,
	const std::map<std::string, int>& params) {




	ss << "__kernel " << std::endl;
	ss << "__attribute__((reqd_work_group_size(" << params.at("TILE_SIZE_W") << ", " << params.at("TILE_SIZE_H") << ", " << params.at("TILE_SIZE_Z") << ")))" << std::endl;
	ss << "void convolution(" << std::endl; 
	ss << "  const __global float * input, " << std::endl; 
	ss << "  __global float* output," << std::endl; 
	ss << "  const __global float* filter) {" << std::endl << std::endl << std::endl; 

	ss << "  __local float in_img_cache[TILE_SIZE_Z][FOOTPRINT_H][FOOTPRINT_W];" << std::endl; 
	ss << "  __private float out_img_cache[TILE_SIZE] = {.0};" << std::endl; 
	ss << "  __private float filter_reg_cache;" << std::endl; 
	
	if (params.at("TRANS_OUTPUT")) {
		ss << "  __local float out_img_trans[TILE_SIZE_Z][TILE_SIZE][TILE_SIZE];" << std::endl; 
	}


	ss << "  int local_row = get_local_id(1);" << std::endl; 
	ss << "  int local_col = get_local_id(0);" << std::endl; 


	ss << "  int dest, destH, destW, src, srcH, srcW;" << std::endl;

	ss << "  for (int batch_off = 0; batch_off < F_IN; batch_off += TILE_SIZE_Z) {" << std::endl;

	    		load_img(ss, params, true);
				conv_compute_unroll(ss, params); 

	ss << "  }" << std::endl; 

	save_result(ss, params);
	
	ss << "}" << std::endl; 
}



std::string conv_codegen(
	int local_dim1, int local_dim2, int local_dim3,
	int output_bucket_size,
	int trans_output,
	int filter_c, int filter_r,
	int f_in, int f_out,
	int in_w, int in_h) {
    


	int half_filter_size = filter_c / 2;
	int filter_size = filter_c * filter_r;


	int input_size = in_w * in_h;

	int out_w = in_w;
	int out_h = in_h;
	int output_size = out_w * out_h;


	int tile_size_w = local_dim1;
	int tile_size_h = local_dim2;
	int tile_size = tile_size_w * tile_size_h;



	int footprint_w = (tile_size_w - 1) + filter_c;
	int footprint_h = (tile_size_h - 1) + filter_r;


	int output_bucket_num = f_out / output_bucket_size;


	int img_load_time = (footprint_w * footprint_h + tile_size - 1) / tile_size;


	std::stringstream ss;

	std::map<std::string, int> params;


	params["TRANS_OUTPUT"] = trans_output? 1 : 0;
	params["HALF_FILTER_SIZE"] = half_filter_size;
	params["FILTER_C"] = filter_c;
	params["FILTER_R"] = filter_r;


	add_def(ss, params, "FILTER_SIZE", filter_size);

	add_def(ss, params, "F_IN", f_in);
	add_def(ss, params, "F_OUT", f_out);

	add_def(ss, params, "IN_W", in_w);
	add_def(ss, params, "IN_H", in_h);
	add_def(ss, params, "INPUT_SIZE", input_size);

	add_def(ss, params, "OUT_W", out_w);
	add_def(ss, params, "OUT_H", out_h);
	add_def(ss, params, "OUTPUT_SIZE", output_size);


	add_def(ss, params, "TILE_SIZE_W", local_dim1);
	add_def(ss, params, "TILE_SIZE_H", local_dim2);
	add_def(ss, params, "TILE_SIZE_Z", local_dim3);

	add_def(ss, params, "TILE_SIZE", tile_size);

	add_def(ss, params, "FOOTPRINT_W", footprint_w);
	add_def(ss, params, "FOOTPRINT_H", footprint_h);


	add_def(ss, params, "OUTPUT_BUCKET_SIZE", output_bucket_size);	
	add_def(ss, params, "IMG_LOAD_TIME", img_load_time);

	conv_code_body(ss, params);


	return ss.str();
}

