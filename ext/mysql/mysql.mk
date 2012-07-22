OBJS+=ext/mysql/mysql.o
LDFLAGS+=-lmysqlclient

ext/mysql/mysql.o: CFLAGS=-Wall -Wextra -pedantic -iquote ./inc -g -fPIC