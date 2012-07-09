SAPIS=$(shell echo */ | xargs basename)
CFLAGS=-Wall -Wextra -pedantic -ansi -Werror -I../../inc/
LDFLAGS=-L../../ -lgc -lslash

.PHONY: clean list

sapi[%]:
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" make -C $*

clean:
	for d in $(SAPIS); do make -C $$d clean; done

list:
	@for d in $(SAPIS); do tput setaf 2; tput bold; echo "make sapi[$$(tput setaf 3)$$d$$(tput setaf 2)]"; tput sgr0; make -C $$d describe | sed -e 's/\(.*\)/- \1/'; echo; done