TOOLCHAIN=$(BASE_DIR)/host/bin
CC := $(TOOLCHAIN)/arm-buildroot-linux-uclibcgnueabihf-gcc
CFLAGS := -lpthread -lwiringPi

OUTPUT := jitter_and_latency

all: $(OUTPUT)

%.o: %.c
	$(CC) -o $@ $< $(CFLAGS)
