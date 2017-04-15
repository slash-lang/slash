include ../../local.mk

CFLAGS += -iquote ../../src/gen # for opcode_enum.inc

.PHONY: clean install uninstall

slash-disasm: slash-disasm.o dis.inc ../../libslash.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDFLAGS)

slash-disasm.o: slash-disasm.c dis.inc

clean:
	rm -f *.o
	rm -rf *.dSYM
	rm -f slash-dis

install:
	cp slash-dis $(DESTDIR)/$(INSTALL_PREFIX)/bin

uninstall:
	rm -f $(DESTDIR)/$(INSTALL_PREFIX)/bin/slash-dis
