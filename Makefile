.SILENT:

.PHONY: build

all: build-all

build:
	mkdir -p build
	cmake -B build

build-all: ./build
	make -C build --no-print-directory

run: build-all
	./build/samples/sim/sim

clean:
	rm -rf build