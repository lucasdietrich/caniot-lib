# Justfile for caniot-lib

# Default recipe
all: build-all

build:
	cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_LIBDIR=out -DCMAKE_INSTALL_PREFIX=. -B build
	make -C build install

build-static:
	cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_LIBDIR=out -DCMAKE_INSTALL_PREFIX=. -B build
	make -C build install

build-all: build
	make -C build --no-print-directory

run-sim: build-all
	./build/samples/sim/sim

run-attr: build-all
	./build/samples/attributes/sample_attr

run-tests: build-all
	./build/tests/test

rust-dyn EXAMPLE="device": build
	cargo run -p caniot --example {{EXAMPLE}}

rust EXAMPLE="device": build-static
	CANIOT_LIB_PATH=out cargo run --example {{EXAMPLE}}

rust-test: build-static
	CANIOT_LIB_PATH=out cargo test

clean:
	rm -rf build target out

format:
	find src -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' | xargs clang-format -i
	find include -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' | xargs clang-format -i
	find tests -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' | xargs clang-format -i
	find samples -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' | xargs clang-format -i
