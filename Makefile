CC = arc-elf32-gcc
LD = arc-elf32-ld
CFLAGS = -ffreestanding -nostdlib
LDFLAGS = -M
ODIR=obj
_OBJ = badApple.o string.o
OBJ  = $(patsubst %,$(ODIR)/%,$(_OBJ))
default: sd

$(OBJ): $(ODIR)/%.o: %.c
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)
elfCart: $(OBJ) $(SOBJ)
	$(LD) -o $@ $^ $(LDFLAGS) -T leapsterCart.ld
elfSD: $(OBJ) $(SOBJ)
	$(LD) -o $@ $^ $(LDFLAGS) -T leapsterSD.ld
cart: elfCart
	./tools/binToRib ./tools/tinyLeapsterRom.bin ./elfCart cart
sd: elfSD
	./tools/binToRib ./tools/SDMenu.bin ./elfSD sd
clean:
	rm -rf $(ODIR)/*
	rm ./helloWorld
