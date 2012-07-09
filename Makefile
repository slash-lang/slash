CFLAGS=-Wall -Wextra -pedantic -Werror -ansi -Ideps/gc-7.2/include -iquote ./inc
LDFLAGS=

OBJS=src/slash.o

.PHONY=clean

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