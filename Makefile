.SILENT:

.PHONY: all no_test programs lib tests install uninstall clean test check covtest lcov apidoc apidoc_clean

all: build run

build-dir:
	mkdir -p build
	cmake -B build

build: build-dir
	make -C build --no-print-directory

run:
	./build/samples/sim/sim

clean:
	rm -rf build