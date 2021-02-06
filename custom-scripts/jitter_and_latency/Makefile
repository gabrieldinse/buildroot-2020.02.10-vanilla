TOOLCHAIN=$(BASE_DIR)/host/bin
CC := $(TOOLCHAIN)/arm-buildroot-linux-uclibcgnueabihf-gcc

OUTPUT := jitter_and_latency
CFLAGS := -lpthread -lwiringPi

all: $(OUTPUT)

%.o: %.c
	$(CC) -o $@ $< $(CFLAGS)
