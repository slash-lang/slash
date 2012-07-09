.PHONY: clean

slash-cli: cli.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^

clean:
	rm -f *.o
	rm -f slash-cli