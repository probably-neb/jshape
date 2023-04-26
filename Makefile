CC=gcc

jasn:
	zig build
	cp ./zig-out/bin/jasn .
