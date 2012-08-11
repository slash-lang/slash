include ../../local.mk

.PHONY: clean install uninstall

slash-cli: cli.o ../../libslash.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f *.o
	rm -rf *.dSYM
	rm -f slash-cli

install:
	cp slash-cli $(INSTALL_PREFIX)/bin

uninstall:
	rm -f $(INSTALL_PREFIX)/bin/slash-cli