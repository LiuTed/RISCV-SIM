FILES = $(patsubst %.c, %, $(wildcard *.c))

all: $(FILES)

$(FILES): %: %.c
	/opt/riscv/bin/riscv64-unknown-linux-gnu-gcc -I../ $< -o $@
clean:
	rm -f $(FILES)
