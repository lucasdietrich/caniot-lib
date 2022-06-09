.SILENT:

.PHONY: all no_test programs lib tests install uninstall clean test check covtest lcov apidoc apidoc_clean

all: build-all run

./build:
	mkdir -p build
	cmake -B build

build-all: ./build
	make -C build --no-print-directory

run: build-all
	./build/samples/sim/sim

clean:
	rm -rf build