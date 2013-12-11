include ../../local.mk

.PHONY: clean install uninstall

slash-cli: cli.o readline.o ../../libslash.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o
	rm -rf *.dSYM
	rm -f slash-cli

install:
	cp slash-cli $(DESTDIR)/$(INSTALL_PREFIX)/bin

uninstall:
	rm -f $(DESTDIR)/$(INSTALL_PREFIX)/bin/slash-cli
