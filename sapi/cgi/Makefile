include ../../local.mk

.PHONY: clean install uninstall

slash-cgi: http_status.o api.o cgi.o ../../libslash.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o
	rm -rf *.dSYM
	rm -f slash-cgi

install:
	cp slash-cgi $(INSTALL_PREFIX)/bin

uninstall:
	rm -f $(INSTALL_PREFIX)/bin/slash-cgi
