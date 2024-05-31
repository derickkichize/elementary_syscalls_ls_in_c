PHONY: clean all

PROG = ls

all:
	make -C src
	mkdir build
	mv ./src/${PROG} ./build/${PROG}

clean:
	make -C src clean
	rm -rf build
	rm -rf *.core

