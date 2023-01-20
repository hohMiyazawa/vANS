#include <stdio.h>

#include "file_io.hpp"
#include "vANS.h"

void print_usage(){
	printf("./raw_bits infile.png outfile.ans\n");
}

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("not enough arguments\n");
		print_usage();
		return 1;
	}

	size_t in_size;
	uint8_t* in_bytes = read_file(argv[1], &in_size);


	static size_t out_max_size = in_size * 20;
	static size_t out_max_elems = out_max_size / sizeof(uint32_t);
	uint32_t* out_buf = new uint32_t[out_max_elems];
	uint32_t* out_end = out_buf + out_max_elems;
	uint8_t* dec_bytes = new uint8_t[in_size];

	vANS_state state = vANS_state_init(out_buf, out_end);

	vANS_stats stats;
	stats.init(256);
	for(size_t i=0;i<in_size;i++){
		stats.add(in_bytes[i]);
	}
	vANS_enc_context context = vANS_create_context(stats, state);


	for(size_t i=in_size;i--;){
		vANS_writeSymbol(in_bytes[i], context, state);
	}

	printf("data %d\n",(int)(state.data_end - state.data));

	vANS_writeContext(context, state);
	uint32_t* start_of_data = vANS_flush(state);

	write_file(argv[2],(uint8_t*)start_of_data,4*(out_end - start_of_data));

	delete[] out_buf;

	return 0;
}
