OBJS+=ext/posix/posix.o

ext/posix/posix.o: CFLAGS += -D_POSIX_SOURCE
