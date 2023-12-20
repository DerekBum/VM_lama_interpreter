all: main.o gc_runtime.o runtime.o
	g++ -g -m32 build/gc_runtime.o build/runtime.o build/main.o -o build/lama_c_interpreter

main.o: build main.cpp runtime.h constants.h
	g++ -g -fstack-protector-all -m32 -c main.cpp -o build/main.o

gc_runtime.o: build gc_runtime.s
	cc -g -fstack-protector-all -m32 -c gc_runtime.s -o build/gc_runtime.o

runtime.o: build runtime.c runtime.h
	cc -g -fstack-protector-all -m32 -c runtime.c -o build/runtime.o

build:
	mkdir build

clean:
	rm -r build/
