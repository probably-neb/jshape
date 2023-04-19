CC=gcc


jasn: src/main.o
	$(CC) -o $@ $^
