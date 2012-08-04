SAPIS=$(shell ls -F | grep "/" | sed -e 's/\///')
CFLAGS=-Wall -Wextra -pedantic -ansi -iquote../../inc/
LDFLAGS=-lpthread -lm
LIBS=../../libslash.a

include ../local.mk

.PHONY: clean list

sapi[%]:
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" LIBS="$(LIBS)" make -C $*

list:
	@for d in $(SAPIS); do tput setaf 2; tput bold; echo "make sapi[$$(tput setaf 3)$$d$$(tput setaf 2)]"; tput sgr0; make -s -C $$d describe | sed -e 's/\(.*\)/- \1/'; echo; done

clean:
	for d in $(SAPIS); do make -C $$d clean; done