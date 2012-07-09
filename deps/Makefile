all: gc-7.2/libgc.la

gc-7.2/libgc.la: gc-7.2/Makefile
	cd gc-7.2 && make

%/Makefile: %/configure
	cd $* && ./configure

gc-7.2/configure:
	curl -o gc-7.2.tar.gz http://www.hpl.hp.com/personal/Hans_Boehm/gc/gc_source/gc-7.2.tar.gz
	tar zfx gc-7.2.tar.gz
	rm gc-7.2.tar.gz