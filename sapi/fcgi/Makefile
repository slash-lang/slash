.PHONY: clean install uninstall

LDFLAGS += -lfcgi -pthread

slash-fcgi: slash-fcgi.c

install: slash-fcgi
	cp slash-fcgi $(INSTALL_PREFIX)/bin

clean:
	rm -f slash-fcgi

uninstall:
	rm -f $(INSTALL_PREFIX)/bin/slash-cli