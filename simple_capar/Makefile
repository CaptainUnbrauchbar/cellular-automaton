CC=$(PREP) mpicc
CFLAGS=-O2 -Wall -pedantic
LDFLAGS=-lcrypto

.PHONY: all clean verify

all: caseq capar

caseq: caseq.c random.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

capar: capar.c random.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

test: caseq capar
	./caseq	1000 0
	mpirun -np 1 ./capar 1000 0
	mpirun -np 2 ./capar 1000 0

verify: caseq capar 
	./caseq	1000 100
	mpirun -np 1 ./capar 1000 100
	mpirun -np 2 ./capar 1000 100
	mpirun -np 3 ./capar 1000 100

clean:
	rm -f capar caseq
