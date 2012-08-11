include local.mk

CFLAGS+=-Wall -Wextra -pedantic -ansi -iquote $(BASE_PATH)/inc -fPIC
LDFLAGS+=-L$(BASE_PATH) -lslash

OBJS+=src/class.o src/error.o src/method.o src/object.o src/st.o src/string.o \
	src/value.o src/vm.o src/lib/int.o src/lib/number.o src/lib/float.o \
	src/lib/bignum.o src/utf8.o src/lex.o src/lex_helper.o src/lib/nil.o \
	src/lib/true.o src/lib/false.o src/eval.o src/parse.o src/parse_helper.o \
	src/eval.o src/lib/array.o src/lib/comparable.o src/lib/require.o \
	src/lib/lambda.o src/lib/enumerable.o src/lib/file.o src/init_exts.o \
	src/lib/rand.o src/lib/dict.o src/lib/request.o src/lib/response.o \
	src/lib/error_page.o src/lib/system.o src/lib/regexp.o src/gc.o \
	src/lib/range.o src/vm_exec.o src/compile.o

SAPIS=$(shell ls -F sapi | grep "/" | sed -e 's/\///')

.PHONY=clean default test install

default: $(TARGETS)

test: sapi[cli]
	sapi/cli/slash-cli test/driver.sl

sapi[%]: libslash.a
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" make -C sapi/$*
	@tput setaf 2; tput bold
	@echo "Built $* successfully"
	@tput sgr0

libslash.a: $(OBJS)
	ar r $@ $^

src/lex.o: CFLAGS += -Wno-unused -Wno-unused-parameter -Wno-sign-compare

%.o: %.c inc/*.h Makefile local.mk
	$(CC) -o $@ $(CFLAGS) -c $<

src/vm_exec.o: src/vm_exec.c src/vm_exec.inc inc/*.h Makefile local.mk
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
	for d in $(SAPIS); do make -C sapi/$$d clean; done

install:
	mkdir -p $(INSTALL_PREFIX)/bin
	mkdir -p $(INSTALL_PREFIX)/lib
	mkdir -p $(INSTALL_PREFIX)/include/slash
	cp libslash.a $(INSTALL_PREFIX)/lib
	cp -r inc/* $(INSTALL_PREFIX)/include/slash
	echo '#include "slash/slash.h"' > $(INSTALL_PREFIX)/include/slash.h
	for sapi in $(SAPIS_ENABLED); do make -C sapi/$$sapi install; done

uninstall:
	rm -f $(INSTALL_PREFIX)/lib/libslash.a
	rm -f $(INSTALL_PREFIX)/include/slash.h
	rm -rf $(INSTALL_PREFIX)/include/slash
	for sapi in $(SAPIS_ENABLED); do make -C sapi/$$sapi uninstall; done
