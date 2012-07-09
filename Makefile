CFLAGS=-Wall -Wextra -pedantic -Werror -ansi -Ideps/gc-7.2/include -iquote ./inc
LDFLAGS=

OBJS=src/slash.o

.PHONY=clean default

default:
	@echo "Please choose something to build. Here are the useful targets:"
	@echo 
	@make -C sapi list
	@tput setaf 2; tput bold; echo "make $$(tput setaf 3)libslash.a"; tput sgr0
	@echo "- Slash static library"
	@echo

sapi[%]: libgc.la libslash.a
	make -C sapi sapi[$*]

libgc.la: deps/gc-7.2/libgc.la
	cp $< $@

libslash.a: $(OBJS)
	ar r $@ $^

%.o: %.c inc/*.h Makefile
	$(CC) -o $@ $(CFLAGS) -c $<

clean:
	rm -f src/*.o
	rm -f libgc.la
	rm -f libslash.a
	make -C sapi clean