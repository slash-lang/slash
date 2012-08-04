CFLAGS=-Wall -Wextra -pedantic -ansi -iquote ./inc -fPIC
LDFLAGS=

OBJS=src/class.o src/error.o src/method.o src/object.o src/st.o src/string.o \
	src/value.o src/vm.o src/lib/int.o src/lib/number.o src/lib/float.o \
	src/lib/bignum.o src/utf8.o src/lex.o src/lex_helper.o src/lib/nil.o \
	src/lib/true.o src/lib/false.o src/eval.o src/parse.o src/parse_helper.o \
	src/eval.o src/lib/array.o src/lib/comparable.o src/lib/require.o \
	src/lib/lambda.o src/lib/enumerable.o src/lib/file.o src/init_exts.o \
	src/lib/rand.o src/lib/dict.o src/lib/request.o src/lib/response.o \
	src/lib/error_page.o src/lib/system.o src/lib/regexp.o src/gc.o

include local.mk

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

sapi[%]: libslash.a
	make -C sapi sapi[$*]
	@tput setaf 2; tput bold
	@echo "Built $* successfully"
	@tput sgr0

ext[%]:
	cd ext/$* && ../../sapi/cli/slash-cli conf.sl

libslash.a: $(OBJS)
	ar r $@ $^

src/lex.o: CFLAGS += -Wno-unused -Wno-unused-parameter -Wno-sign-compare

%.o: %.c inc/*.h Makefile local.mk
	$(CC) -o $@ $(CFLAGS) -c $<

%.c: %.yy inc/*.h Makefile local.mk
	flex -o $@ $<

src/lib/error_page.o: src/lib/error_page.sl
	perl scripts/txt-to-c.pl sl__error_page_src < $< > $@.c
	$(CC) -o $@ -c $@.c
	rm -f $@.c

clean:
	rm -f src/*.o
	rm -f src/lib/*.o
	rm -f ext/*/*.o
	rm -f platform/*.o
	rm -f libslash.a
	make -C sapi clean

clean-all: clean
	make -C deps clean