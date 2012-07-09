.PHONY: clean describe

slash-cli: cli.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^

clean:
	rm -f *.o
	rm -f slash-cli

describe:
	@echo "Command Line Interface for Slash"