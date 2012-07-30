.PHONY: clean describe

APR=apr-1
APXS=apxs

SO_EXT=$(shell $(APR)-config --apr-so-ext)

mod_slash.so: .libs/mod_slash.so
	cp $< $@

.libs/mod_slash.so: mod_slash.c $(LIBS)
	$(APXS) -n mod_slash "-Wc,$(CFLAGS)" "-Wl,$(LDFLAGS)" -c $^

clean:
	rm -f *.o *.so *.$(SO_EXT)
	rm -rf *.dSYM .libs

describe:
	@echo "Embeds a Slash interpreter in an apache2 process"