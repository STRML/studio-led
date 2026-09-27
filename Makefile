SMC = smc.c -framework IOKit -framework CoreFoundation
BINS = studio-led smcdump smcwrite

all: $(BINS)

$(BINS): %: %.c smc.c smc.h
	clang -O2 -Wall -o $@ $< $(SMC)

clean:
	rm -f $(BINS)
