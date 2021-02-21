TOOLCHAIN=$(BASE_DIR)/host/bin
CC := $(TOOLCHAIN)/arm-linux-gnueabihf-gcc
CFLAGS := -lpthread -lgpiod

OUTPUT := jitter_and_latency

all: $(OUTPUT)

%.o: %.c
	$(CC) -o $@ $< $(CFLAGS)
