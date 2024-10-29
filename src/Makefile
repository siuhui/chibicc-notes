CFLAG=-std=c11 -g

chibicc: main.o
	$(CC) -o chibicc main.o

test: chibicc
	./test.sh

clean:
	rm -f chibicc *.o tmp*

.PHONY: test clean
