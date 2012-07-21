.PHONY: clean describe

APR=apr-1
APXS=apxs

SO_EXT=$(shell $(APR)-config --apr-so-ext)

mod_slash.$(SO_EXT): mod_slash.c $(LIBS)
	$(APXS) -n mod_slash "-Wc,$(CFLAGS) $(LDFLAGS)" -c $^

clean:
	rm -f *.o *.so
	rm -rf *.dSYM .libs

describe:
	@echo "Embeds a Slash interpreter in an apache2 process"