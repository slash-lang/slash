include ../../local.mk

.PHONY: clean install uninstall

APR=apr-1
APXS=apxs

SO_EXT=$(shell $(APR)-config --apr-so-ext)

mod_slash.so: mod_slash.c ../../libslash.a
	LTFLAGS="--tag=CC" $(APXS) -c -S CC=$(CC) "-Wc,$(CFLAGS)" "-Wl,$(LDFLAGS)" mod_slash.c
	cp .libs/mod_slash.so .

clean:
	rm -f *.o *.so *.$(SO_EXT)
	rm -rf *.dSYM .libs

install: mod_slash.so
	$(APXS) -i -a mod_slash.so

uninstall:
