caniot.o: ./src/caniot.c ./include/caniot.h
	gcc -c ./src/caniot.c -o ./build/caniot.o -I./include

caniot.a: caniot.o
	ar -rcs ./build/caniot.a ./build/caniot.o

main: ./examples/main.c caniot.a
	gcc ./examples/main.c ./build/caniot.a -o build/main -I./include

clean:
	rm -rf ./build/*.c