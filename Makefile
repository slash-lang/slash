CFLAGS=-Wall -Wextra -pedantic -Werror -ansi -Ideps/gc-7.2/include \
	-iquote ./inc -g -Ideps/gmp-5.0.5 -fPIC
LDFLAGS=

PLATFORM=posix

OBJS=src/class.o src/error.o src/method.o src/object.o src/st.o src/string.o \
	src/value.o src/vm.o src/lib/int.o src/lib/number.o src/lib/float.o \
	src/lib/bignum.o src/utf8.o src/lex.o src/lex_helper.o src/lib/nil.o \
	src/lib/true.o src/lib/false.o src/eval.o src/parse.o src/parse_helper.o \
	src/eval.o src/lib/io.o src/lib/array.o src/lib/comparable.o \
	src/lib/require.o

OBJS += platform/$(PLATFORM).o

.PHONY=clean default test

default:
	@echo "Please choose something to build. Here are the useful targets:"
	@echo 
	@make -s -C sapi list
	@tput setaf 2; tput bold; echo "make $$(tput setaf 3)libslash.a"; tput sgr0
	@echo "- Slash static library"
	@echo

test: sapi[cli]
	sapi/cli/slash-cli test/driver.sl

sapi[%]: libgc.a libgmp.a libslash.a
	make -C sapi sapi[$*]
	@tput setaf 2; tput bold
	@echo "Built $* successfully"
	@tput sgr0

libgc.a: deps/gc-7.2/.libs/libgc.a
	cp $< $@

libgmp.a: deps/gmp-5.0.5/.libs/libgmp.a
	cp $< $@

deps/%:
	make -C deps $*

libslash.a: $(OBJS)
	ar r $@ $^

src/lex.o: CFLAGS += -Wno-unused -Wno-unused-parameter -Wno-sign-compare

%.o: %.c inc/*.h Makefile
	$(CC) -o $@ $(CFLAGS) -c $<

%.c: %.yy inc/*.h Makefile
	flex -o $@ $<

clean:
	rm -f src/*.o
	rm -f src/lib/*.o
	rm -f platform/*.o
	rm -f libgmp.a
	rm -f libgc.a
	rm -f libslash.a
	make -C sapi clean

clean-all: clean
	make -C deps clean