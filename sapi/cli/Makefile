.PHONY: clean

slash-cli: cli.o $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o
	rm -rf *.dSYM
	rm -f slash-cli