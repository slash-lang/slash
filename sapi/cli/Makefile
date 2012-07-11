.PHONY: clean describe

slash-cli: cli.o $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o
	rm -f slash-cli

describe:
	@echo "Command Line Interface for Slash"