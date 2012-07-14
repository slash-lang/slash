CFLAGS=-Wall -Wextra -pedantic -Werror -ansi -Ideps/gc-7.2/include \
	-iquote ./inc -g
LDFLAGS=

OBJS=src/class.o src/error.o src/method.o src/object.o src/st.o src/string.o \
	src/value.o src/vm.o src/lib/int.o src/lib/number.o src/lib/float.o \
	src/lib/bignum.o src/lexer.o src/utf8.o

.PHONY=clean default

default:
	@echo "Please choose something to build. Here are the useful targets:"
	@echo 
	@make -C sapi list
	@tput setaf 2; tput bold; echo "make $$(tput setaf 3)libslash.a"; tput sgr0
	@echo "- Slash static library"
	@echo

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

%.o: %.c inc/*.h Makefile
	$(CC) -o $@ $(CFLAGS) -c $<

clean:
	rm -f src/*.o
	rm -f src/lib/*.o
	rm -f libgmp.a
	rm -f libgc.a
	rm -f libslash.a
	make -C sapi clean

clean-all: clean
	make -C deps clean