include local.mk

CFLAGS+=-std=c99 -I $(BASE_PATH)/inc
WARNING_CFLAGS=-Wall -Wextra -pedantic
LDFLAGS+=-L$(BASE_PATH) -lslash

OBJS+=src/class.o src/error.o src/method.o src/object.o src/dispatch.o \
	src/st.o src/string.o src/value.o src/vm.o src/lib/int.o src/lib/number.o \
	src/lib/float.o src/lib/bignum.o src/utf8.o src/lex.o src/lex_helper.o \
	src/lib/nil.o src/lib/true.o src/lib/false.o src/eval.o src/parse.o \
	src/parse_helper.o src/eval.o src/lib/array.o src/lib/comparable.o \
	src/lib/require.o src/lib/lambda.o src/lib/enumerable.o src/lib/file.o \
	src/init_exts.o src/lib/rand.o src/lib/dict.o src/lib/request.o \
	src/lib/response.o src/lib/error_page.o src/lib/system.o src/lib/regexp.o \
	src/gc.o src/lib/range.o src/vm_exec.o src/compile.o src/lib/time.o \
	src/unicode.o src/lib/gc.o src/lib/buffer.o

SAPIS=$(shell ls -F sapi | grep "/" | sed -e 's/\///')

.PHONY=clean default test install

default: $(TARGETS)

fast-test: sapi[cli]
	@sapi/cli/slash-cli test/test.sl test/*/*.sl

test: sapi[cli]
	@echo "Running tests"
	@sapi/cli/slash-cli -I ./lib/slash test/test.sl test/*/*.sl
	@echo "Running tests with --gc-after-test"
	@sapi/cli/slash-cli -I ./lib/slash test/test.sl --gc-after-test test/*/*.sl

sapi[%]: libslash.a
	@echo "make sapi/$*"
	@CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" make -C sapi/$*
	@echo
	@echo "   Built $* successfully"
	@echo

libslash.a: $(OBJS)
	@rm -f $@
	@echo "ar   $@"
	@$(AR) r $@ $^

src/lex.o: CFLAGS += -Wno-unused -Wno-unused-parameter -Wno-sign-compare

%.o: %.c inc/*.h inc/*/*.h inc/*/*/*.h Makefile local.mk
	@echo "cc   $<"
	@$(CC) -o $@ $(WARNING_CFLAGS) $(CFLAGS) -c $<

src/vm_exec.o: src/vm_exec.c src/vm_defn.inc inc/*.h Makefile local.mk

%.c: %.yy inc/*.h Makefile local.mk
	@echo "flex $<"
	@flex -o $@ $<

%.c: %.sl
	@perl scripts/txt-to-c.pl $< > $@

clean:
	rm -f src/*.o
	rm -f src/lib/*.o
	rm -f ext/*/*.o
	rm -f platform/*.o
	rm -f libslash.a
	find . | grep -P '(gcov|gcda|gcno)' | xargs rm -f
	for d in $(SAPIS); do make -C sapi/$$d clean; done

install:
	mkdir -p $(INSTALL_PREFIX)/bin
	mkdir -p $(INSTALL_PREFIX)/lib
	mkdir -p $(INSTALL_PREFIX)/include
	cp libslash.a $(INSTALL_PREFIX)/lib
	cp -r lib/* $(INSTALL_PREFIX)/lib
	cp -r inc/* $(INSTALL_PREFIX)/include
	for sapi in $(SAPIS_ENABLED); do make -C sapi/$$sapi install; done

uninstall:
	rm -f $(INSTALL_PREFIX)/lib/libslash.a
	rm -f $(INSTALL_PREFIX)/include/slash.h
	rm -rf $(INSTALL_PREFIX)/include/slash
	for sapi in $(SAPIS_ENABLED); do make -C sapi/$$sapi uninstall; done
