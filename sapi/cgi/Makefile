include ../../local.mk

.PHONY: clean install uninstall test

CFLAGS+=-std=c99 -I $(BASE_PATH)/inc -D_XOPEN_SOURCE
LDFLAGS+=-lfcgi

CGI_OBJS=http_status.o api.o

slash-cgi: $(CGI_OBJS) cgi.o ../../libslash.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDFLAGS)

slash-cgi-testrunner: $(CGI_OBJS) cgi.c cgi-test.o ../../libslash.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CGI_OBJS) cgi-test.o ../../libslash.a $(LDFLAGS)

test: slash-cgi-testrunner
	@echo "Running tests"
	@./slash-cgi-testrunner -I ../../lib/slash test/runner/test.sl $(BASE_PATH)/sapi/cgi  test/runner/test/*.sl
	@echo "Running tests with --gc-after-test"
	@./slash-cgi-testrunner -I ../../lib/slash test/runner/test.sl --gc-after-test $(BASE_PATH)/sapi/cgi  test/runner/test/*.sl

clean:
	rm -f *.o
	rm -rf *.dSYM
	rm -f slash-cgi

install:
	cp slash-cgi $(INSTALL_PREFIX)/bin

uninstall:
	rm -f $(INSTALL_PREFIX)/bin/slash-cgi
