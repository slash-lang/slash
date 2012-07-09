.PHONY: clean

all: gc-7.2/libgc.la gmp-5.0.5/libgmp.la

%/Makefile: %/configure
	@echo "Configuring dependency $* (logging to configure-$*.log)"
	@cd $* && ./configure &> ../configure-$*.log

gc-7.2/libgc.la: gc-7.2/Makefile
	@echo "Making dependency gc-7.2 (logging to make-gc-7.2.log)"
	@make -C gc-7.2 &> make-gc-7.2.log

gmp-5.0.5/libgmp.la: gmp-5.0.5/Makefile
	@echo "Making dependency gmp-5.0.5 (logging to make-gmp-5.0.5.log)"
	@make -C gmp-5.0.5 &> make-gmp-5.0.5.log

%/configure: %.tar.*
	tar zfx $<

gc-7.2.tar.gz:
	curl -o $@ http://www.hpl.hp.com/personal/Hans_Boehm/gc/gc_source/gc-7.2.tar.gz

gmp-5.0.5.tar.xz:
	curl -o $@ ftp://ftp.gmplib.org/pub/gmp-5.0.5/gmp-5.0.5.tar.xz

clean:
	rm -rf gc-7.2 gmp-5.0.5