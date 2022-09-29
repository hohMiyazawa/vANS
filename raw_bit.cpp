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

	vANS_stats stats;
	stats.init(256);
	for(size_t i=0;i<in_size;i++){
		stats.add(in_bytes[i]);
	}

	vANS_enc_context context = create_enc_tree(stats);

	static size_t out_max_size = in_size * 2;
	static size_t out_max_elems = out_max_size / sizeof(uint32_t);
	uint32_t* out_buf = new uint32_t[out_max_elems];
	uint32_t* out_end = out_buf + out_max_elems;
	uint8_t* dec_bytes = new uint8_t[in_size];

	vANS_state state;
	vANS_enc_init(&state, out_buf, out_end, 20);

	for(size_t i=in_size;i--;){
		vANS_writeSymbol(in_bytes[i], context, &state);
	}

	vANS_writeContext(context, &state);
	vANS_flush(state);

	return 0;
}
