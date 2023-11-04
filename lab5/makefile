run: single double triple triple_nolock
	./single
	./double
	./triple
	./triple_nolock

all: single double triple triple_nolock
	make single double triple triple_nolock

single: single.c
	gcc -o single single.c -lm

double: double.c
	gcc -o double double.c -lm

triple: triple.c
	gcc -o triple triple.c -lm -lpthread

triple_nolock: triple_nolock.c
	gcc -o triple_nolock triple_nolock.c -lm

clean:
	rm -f single double triple triple_nolock
