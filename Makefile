LIBS=-lm -lrt

all: raw_bit

raw_bit: raw_bit.cpp vANS.h rans64.h
	g++ -o $@ $< -O2 $(LIBS)
