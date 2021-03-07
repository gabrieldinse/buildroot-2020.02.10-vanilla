TOOLCHAIN=$(BASE_DIR)/host/bin
CC := $(TOOLCHAIN)/arm-linux-gnueabihf-gcc
CFLAGS := -lpthread -lgpiod

OUTPUT := jitter_and_latency jitter_and_latency_extern

all: $(OUTPUT)

%.o: %.c
	$(CC) -o $@ $< $(CFLAGS)
