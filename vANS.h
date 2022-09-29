#pragma once

#include "rans64.h"
#include <cmath>
#include <math.h>

static const size_t prob_bits = 24;
static const size_t prob_scale = 1 << prob_bits;

typedef struct {
	bool isLeaf;
	symbol_enc_node* left;
	symbol_enc_node* right;
	uint32_t base_val;
	uint32_t cum_freq;
	uint32_t range;
	Rans64EncSymbol esym;
} symbol_enc_node;

typedef struct {
	bool isLeaf;
	symbol_dec_node* left;
	symbol_dec_node* right;
	uint32_t base_val;
	uint32_t cum_freq;
	uint32_t range;
	Rans64DecSymbol dsym;
} symbol_dec_node;

typedef struct {
	uint8_t context_mode;
	uint32_t range;
	bool list_mode;
	symbol_enc_node* root;
	symbol_enc_node* list_root;
} vANS_enc_context;

typedef struct {
	uint8_t context_mode;
	uint32_t range;
	bool list_mode;
	symbol_dec_node* root;
	symbol_enc_node* list_root;
} vANS_dec_context;

typedef struct {
	uint32_t* data;
	uint32_t* data_end;
	uint32_t* data_start;
	Rans64State rans_state;
	uint32_t prob_bits;
	uint32_t prob_scale;
	uint64_t buffer;
	uint8_t buffer_size;
} vANS_state;

typedef struct vANS_stats_dynamic_node;

struct vANS_stats_dynamic_node {
	bool isLeaf;
	vANS_stats_dynamic_node* left;
	vANS_stats_dynamic_node* right;
	uint32_t base_val;
	uint32_t range;
	uint32_t count;
};

struct vANS_stats{
	vANS_stats_dynamic_node* tree;
	uint32_t* list;
	bool mode;
	size_t total;
	uint32_t range;
	void count_freqs(uint8_t const* in, size_t nbytes);
	double estimate_size();
	void init(uint32_t range);
	void add(uint32_t value);
	void remove(uint32_t value);
};

size_t symbol_quant(value){
	return value;//TODO quant
}

double symbol_cost(value){
	return std::log2(value) * value;
}

double symbol_cost_quant(value,quant){
	return value;//TODO closed form
}

vANS_recursive_build_context(
	vANS_stats_dynamic_node* tree,
	symbol_enc_node* root,
	size_t total,
	size_t running_total,
	size_t scaled_total
){
	if(*tree.isLeaf){
		*root.isLeaf = true;
		*root.left = new symbol_enc_node;
		*root.right = new symbol_enc_node;
		vANS_recursive_build_context(stats.tree.left, &context.root.left, total, running_total, scaled_total);
		vANS_recursive_build_context(stats.tree.right, &context.root.right, total, running_total, scaled_total);
	}
	else{
		*root.isLeaf = false;
		//TODO scale freq in leaf node
	}
}
	

vANS_enc_context create_enc_tree(vANS_stats stats){
	vANS_enc_context context;
	context.mode = stats.mode;
	context.range = stats.range;
	if(context.mode){
		vANS_recursive_build_context(stats.tree, &context.root, stats.total, 0, 0);
	}
	else{
		context.list_root = new symbol_enc_node[stats.range];
		size_t running_total = 0;
		size_t scaled_total = 0;
		for(size_t i=0;i<stats.range;i++){
			context.list_root[i].isLeaf = true;
			context.list_root[i].base_val = i;
			context.list_root[i].cum_freq = scaled_total;
			running_total += symbol_quant(stats.list[i]);
			scaled_total = ((uint64_t)prob_scale * (uint32_t)running_total)/((uint32_t)stats.total);
			context.list_root[i].range = 1;
			Rans64EncSymbolInit(
				&context.list_root[i].esym,
				context.list_root[i].cum_freq,
				scaled_total - context.list_root[i].cum_freq,
				prob_bits
			);
		}
	}
}

vANS_stats::init(uint32_t n){
	range = n;
	total = 0;
	if(range < (1<<16)){
		mode = 0;
		list = new uint32_t[n]{};
	}
	else{
		mode = 1;
		vANS_stats_dynamic_node* stat = new vANS_stats_dynamic_node;
		stat->isLeaf = true;
		stat->count = 0;
		stat->base_val = 0;
		uint32_t halver = n - 1;
		stat->range = 1;
		while(halver){
			halver = halver>>1;
			stat->range = stat->range<<1;
		}
		list = stat;
	}
}

vANS_stats::add(uint32_t val){
	count++;
	if(mode){
		vANS_stats_dynamic_node* root = tree;
		while(root->range > 1){
			root->count++;
			if(root->isLeaf){
				root->isLeaf = false;
				vANS_stats_dynamic_node* left  = new vANS_stats_dynamic_node;
				vANS_stats_dynamic_node* right = new vANS_stats_dynamic_node;
				uint32_t new_range = root->range/2;
				left->isLeaf = true;
				right->isLeaf = true;
				left->base_val = root->base_val;
				rigth->base_val = root->base_val + new_range;
				left->count = 0;
				right->count = 0;
				left->range = new_range;
				right->range = new_range;
			}
			if(val < root->range.right->base_val){
				root = root->right;
			}
			else{
				root = root->left;
			}
		}
		root->count++;
	}
	else{
		list[val]++;
	}
}

uint32_t vANS_readBits(
	uint8_t bits,
	vANS_state& vANS
){
	if(bits == 0){
		return 0;
	}
	if(bits > vANS.buffer_size){
		vANS.buffer += (*(vANS.data++)) << vANS.buffer_size;
		vANS.buffer_size += 32;
	}
	uint32_t value = vANS.buffer % (1 << bits);
	vANS.buffer >> bits;
	vANS.buffer_size -= bits;
	return value;
}

uint32_t vANS_readSymbol(
	vANS_context& context,
	vANS_state& vANS
){
}

uint32_t vANS_writeSymbol(
	uint32_t value,
	vANS_context& context,
	vANS_state& vANS
){
}

	

void vANS_writeBits(
	uint8_t bits,
	uint32_t value,
	vANS_state& vANS
){
	if(bits == 0){
		return;
	}
	vANS.buffer = vANS.buffer << bits + value;
	vANS.buffer_size += bits;
	if(vANS.buffer_size > 31){
		vANS.buffer_size = vANS.buffer_size - 32;
		*(--vANS.data) = vANS.buffer >> vANS.buffer_size;
		vANS.buffer = vANS.buffer % (1 << vANS.buffer_size);
	} 
}

vANS_writeContext(
	vANS_enc_context context,
	vANS_state& vANS){
}

uint32_t* vANS_flush(
	vANS_state& vANS
){
	Rans64EncFlush(vANS.rans_state, vANS.data);
	vANS_writeBits(1,1);
	*(--vANS.data) = vANS.buffer << (32 - vANS.buffer_size);
	return vANS.data;
}

vANS_state vANS_init_read(
	uint32_t* data_end,
	uint32_t* data_start,
){
	vANS_state vANS = new vANS_state;
	vANS.data_end = data_end;
	vANS.data_start = data_start;
	vANS.data = data_start;
	vANS.buffer = (*(vANS.data++));
	vANS.buffer_size = 32;
	while(vANS_readBits(1,vANS) == 0){
		//remove padding
	}
	//TODO init rans state
}
