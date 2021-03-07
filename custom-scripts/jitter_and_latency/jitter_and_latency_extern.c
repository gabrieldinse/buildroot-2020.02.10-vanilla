#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <pthread.h>

#include <gpiod.h>


#ifndef CONSUMER
#define CONSUMER "Consumer"
#endif


const char *chipname = "gpiochip0";
struct gpiod_chip *chip;
struct gpiod_line *led;
struct gpiod_line *interrupt;
struct gpiod_line_event event;

int led_pin = 22; 
int interrupt_pin = 24; 


void *led_interrupt(void *arg)
{
    int ret, value = 0;

    while (1)
    {
        ret = gpiod_line_event_wait(interrupt, NULL); 
        if (ret > 0)
        {
            ret = gpiod_line_event_read(interrupt, &event);
            if (ret < 0)
            {
                perror("Error: reading last GPIO interrupt failed\n");
                gpiod_line_release(interrupt);
                gpiod_line_release(led);
                gpiod_chip_close(chip);
            }

            ret = gpiod_line_set_value(led, value);
            if (ret < 0)
            {
                perror("Error: writing to led pin failed\n");
                gpiod_line_release(interrupt);
                gpiod_line_release(led);
                gpiod_chip_close(chip);
            }

            value = !value;
        }
        else if (ret < 0)
        {
            perror("Error: waiting for event notification failed\n");
            gpiod_line_release(interrupt);
            gpiod_line_release(led);
            gpiod_chip_close(chip);
            exit(1);
        }
    }
}


void setup_gpio()
{
    int ret;

    chip = gpiod_chip_open_by_name(chipname);
    if (!chip)
    {
        perror("Error: Opening chip GPIO failed\n");
        exit(1);
    }

    led = gpiod_chip_get_line(chip, led_pin);
    if (!led)
    {
        perror("error: getting gpio led line failed\n");
        gpiod_chip_close(chip);
        exit(1);
    }

    interrupt = gpiod_chip_get_line(chip, interrupt_pin);
    if (!interrupt)
    {
        perror("error: getting gpio interrupt line failed\n");
        gpiod_chip_close(chip);
        exit(1);
    }

    ret = gpiod_line_request_output(led, CONSUMER, 0);
    if (ret < 0)
    {
        perror("Error: requesting led line as output failed\n");
        gpiod_line_release(led);
        gpiod_chip_close(chip);
        exit(1);
    }

    ret = gpiod_line_request_both_edges_events(interrupt, CONSUMER);
    if (ret < 0)
    {
        perror("Error: requesting interrupt failed\n");
        gpiod_line_release(interrupt);
        gpiod_line_release(led);
        gpiod_chip_close(chip);
        exit(1);
    }
}


int main(int argc, char *argv[])
{
    pthread_t task_interrupt;

    setup_gpio();
    
    pthread_create(&task_interrupt, NULL, led_interrupt, NULL);

    printf("Press any key to stop . . .\n");
    getchar();
    gpiod_line_release(interrupt);
    gpiod_line_release(led);
    gpiod_chip_close(chip);
    return 0;
}
