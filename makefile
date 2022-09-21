#**************
# Hayden Coffey
# 
#**************

targets=./bin \
		./bin/pipe \
		./bin/socket \
		./bin/shared

all: $(targets)

.PHONY: clean

C=clang

flags=-I./inc -fPIE

cxxFlags=$(shell llvm-config --cxxflags)
ldFlags=$(shell llvm-config --ldflags --libs)

./bin:
	mkdir $@

%.o: %.c
	$(C) -c -o $@ $< $(flags)

./bin/pipe: ./src/pipe.o ./src/util.o
	$(C) -o $@ $^ $(flags)

./bin/socket: ./src/socket.o ./src/util.o
	$(C) -o $@ $^ $(flags)

./bin/shared: ./src/shared.o ./src/util.o
	$(C) -o $@ $^ $(flags) -lpthread -g

#%.ll: %.cc
#	$(CUSTOM_CC) -o $@ $< -std=c++14 $(cxxFlags) $(flags) -S -emit-llvm -fverbose-asm



#tool.so: tool.o
#	$(CXX) -shared -fPIC -o $@ $< $(cxxFlags) $(ldFlags) $(flags)

#src_dir: export CFLAGS=-DCUSTOM_MACRO
#src_dir:
#	$(MAKE) -C src_dir

#clean:
#	$(MAKE) -C src_dir clean

clean:
	rm -rf ./src/*.o ./src/*.ll ./bin

