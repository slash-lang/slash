OBJS+=ext/json/json.o
LDFLAGS+=-lyajl

ext/json/json.o: CFLAGS += -std=c99