SAPIS=$(shell ls -F | grep "/" | sed -e 's/\///')
CFLAGS=-Wall -Wextra -pedantic -ansi -iquote../../inc/
LDFLAGS=-lpthread -lm
LIBS=../../libslash.a

include ../local.mk

.PHONY: clean

sapi[%]:
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" LIBS="$(LIBS)" make -C $*

clean:
	for d in $(SAPIS); do make -C $$d clean; done