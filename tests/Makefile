FILES = $(patsubst %.c, %, $(wildcard *.c))
OPT_FILE = large-qsort.c mat-mul.c ackermann.c

all: $(FILES)

$(FILES): %: %.c
	/opt/riscv/bin/riscv64-unknown-linux-gnu-gcc -I../ $< -o $@ $(if $(filter $<, $(OPT_FILE)), -O2, -O0)

clean:
	rm -f $(FILES)
