include ../../local.mk

.PHONY: clean install uninstall

slash-server: server.o mongoose.o ../../libslash.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o
	rm -rf *.dSYM
	rm -f slash-server

install:
	cp slash-server $(INSTALL_PREFIX)/bin

uninstall:
	rm -f $(INSTALL_PREFIX)/bin/slash-server