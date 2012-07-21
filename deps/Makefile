.PHONY: clean

all: gc-7.2/.libs/libgc.a gmp-5.0.5/.libs/libgmp.a

gc-7.2/.libs/libgc.a: gc-7.2.tar.gz
	tar zfx gc-7.2.tar.gz
	@echo "Configuring dependency gc-7.2 (logging to configure-gc-7.2.log)"
	@cd gc-7.2 && ./configure &> ../configure-gc-7.2.log
	@echo "Making dependency gc-7.2 (logging to make-gc-7.2.log)"
	@make -C gc-7.2 &> make-gc-7.2.log

gmp-5.0.5/.libs/libgmp.a: gmp-5.0.5.tar.bz2
	bzip2 -d gmp-5.0.5.tar.bz2
	tar fx gmp-5.0.5.tar
	@echo "Configuring dependency gmp-5.0.5 (logging to configure-gmp-5.0.5.log)"
	@cd gmp-5.0.5 && ./configure --enable-static --disable-shared --with-pic &> ../configure-gmp-5.0.5.log
	@echo "Making dependency gmp-5.0.5 (logging to make-gmp-5.0.5.log)"
	@make -C gmp-5.0.5 &> make-gmp-5.0.5.log

gc-7.2.tar.gz:
	curl -o $@ http://www.hpl.hp.com/personal/Hans_Boehm/gc/gc_source/gc-7.2.tar.gz

gmp-5.0.5.tar.bz2:
	curl -o $@ ftp://ftp.gmplib.org/pub/gmp-5.0.5/gmp-5.0.5.tar.bz2

clean:
	rm -rf gc-7.2 gmp-5.0.5
