SAPIS=$(shell echo */ | xargs basename)
CFLAGS=-Wall -Wextra -pedantic -ansi -Werror -I../../inc/
LDFLAGS=-L../../ -lgc -lslash

.PHONY: clean

sapi[%]:
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" make -C $*

clean:
	for d in $(SAPIS); do make -C $$d clean; done