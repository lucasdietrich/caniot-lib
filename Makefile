# http://www.gnu.org/software/make/manual/make.html

# https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
# http://perso.univ-lyon1.fr/jean-claude.iehl/Public/educ/Makefile.html
# https://stackoverflow.com/questions/53136024/makefile-to-compile-all-c-files-without-needing-to-specify-them/53138757

SRC  = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,build/%.o,$(SRC))
DEP  = $(patsubst src/%.c,build/%.d,$(SRC))

INC = $(wildcard include/*.h)

CFLAGS = -Wall
INC_OPT = -Iinclude/

build/%.o: src/%.c
	gcc -o $@ -c $< $(CFLAGS) $(INC_OPT) -MMD

-include $(DEP)

build/caniot.a: $(OBJ)
	ar -rcs $@ $^

APP_SRC = $(wildcard examples/*.c)
APP_DEP  = $(patsubst examples/%.c,build/%.d,$(APP_SRC))

APP_INC = -Iexamples/ $(INC_OPT)

build/main: $(APP_SRC) build/caniot.a
	gcc -o $@ $^ $(CFLAGS) $(APP_INC) -MMD

-include $(APP_DEP)

echo:
	echo $(APP_DEP)

run: build/main
	./build/main

clean:
	rm -rf ./build/*