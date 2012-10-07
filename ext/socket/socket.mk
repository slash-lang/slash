OBJS+= ext/socket/socket.o

ext/socket/socket.o: CFLAGS += -D_POSIX_SOURCE