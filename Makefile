.SILENT:

.PHONY: build

all: build-all

build:
	mkdir -p build
	cmake -B build

build-all: ./build
	make -C build --no-print-directory

run-sim: build-all
	./build/samples/sim/sim

run-attr: build-all
	./build/samples/attributes/sample_attr

run-tests: build-all
	./build/tests/test

clean:
	rm -rf build

format:
	find src -iname *.h -o -iname *.c -o -iname *.cpp | xargs clang-format -i
	find include -iname *.h -o -iname *.c -o -iname *.cpp | xargs clang-format -i
	find tests -iname *.h -o -iname *.c -o -iname *.cpp | xargs clang-format -i
	find samples -iname *.h -o -iname *.c -o -iname *.cpp | xargs clang-format -i