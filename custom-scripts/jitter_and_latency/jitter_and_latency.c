#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <pthread.h>
#include <semaphore.h>

#include <gpiod.h>


#define NUM_SAMPLES 100000

#ifndef CONSUMER
#define CONSUMER "Consumer"
#endif


intmax_t times_blink[NUM_SAMPLES];
intmax_t times_interrupt[NUM_SAMPLES];
struct timespec sleep_amount = {0, 250000L}; // 250us
struct timespec interrupt_timeout = {3, 0L};
struct timespec ts1;
struct timespec ts2;

const char *chipname = "gpiochip0";
struct gpiod_chip *chip;
struct gpiod_line *led;
struct gpiod_line *interrupt;
struct gpiod_line_event event;

int led_pin = 22; // gpio pin 22
int interrupt_pin = 24; // gpio pin 24

volatile int interrupt_count = 0;


void *blink_led(void *arg)
{
    int i;
    for (i = 0; i < NUM_SAMPLES; ++i)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        gpiod_line_set_value(led, 1);
        times_blink[i] = ts1.tv_sec * 1e9 + ts1.tv_nsec;
        nanosleep(&sleep_amount, NULL);
        gpiod_line_set_value(led, 0);
        nanosleep(&sleep_amount, NULL);
    }
}


void *led_interrupt(void *arg)
{
    int ret, i;

    while (1)
    {
        ret = gpiod_line_event_wait(interrupt, &interrupt_timeout);
        clock_gettime(CLOCK_MONOTONIC, &ts2);
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

            times_interrupt[interrupt_count] = ts2.tv_sec * 1e9 + ts2.tv_nsec;
            interrupt_count++;
        }
        else if (ret == 0)
        {
            printf("No more interrupts to read (timeout)...\n");
            break;
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


intmax_t* calc_time_diff(intmax_t *times_blink, intmax_t *times_interrupt, int num_of_samples)
{
    int i;
    intmax_t *time_diff = (intmax_t *)malloc(NUM_SAMPLES * sizeof(intmax_t));
    for (i = 0; i < num_of_samples; ++i)
    {
        time_diff[i] = times_interrupt[i] - times_blink[i];
    }
    return time_diff;
}


intmax_t calc_average_time(int num_of_samples, intmax_t *time_diff)
{
    intmax_t average = 0;
    int i;
    for (i = 0; i < num_of_samples; ++i)
    {
        average += time_diff[i];
    }
    return average / num_of_samples;
}


void create_time_diffs_csv(char * filename, intmax_t number_of_values,
        intmax_t *time_diff){
    unsigned int n=0;
    FILE *file;
    file = fopen(filename,"w");
    while (n < number_of_values)
    {
       fprintf(file,"%u,%lld\n",n,time_diff[n]);
       n++;
    } 
    fclose(file);
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

    ret = gpiod_line_request_rising_edge_events(interrupt, CONSUMER);
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
    pthread_t task_led;
    pthread_t task_interrupt;
    intmax_t *time_diff;
    intmax_t average;

    setup_gpio();
    
    pthread_create(&task_interrupt, NULL, led_interrupt, NULL);
    pthread_create(&task_led, NULL, blink_led, NULL);
    printf("Started blinking led...\n");
    pthread_join(task_led, NULL);
    pthread_join(task_interrupt, NULL);

    printf("Calculating time differences...\n");
    time_diff = calc_time_diff(times_blink, times_interrupt, NUM_SAMPLES);
    printf("Writing time differentes to csv...\n");
    create_time_diffs_csv("/root/time_diff.csv", NUM_SAMPLES, time_diff);
    printf("Calculating average...\n");
    average = calc_average_time(NUM_SAMPLES, time_diff);
    free(time_diff);
    printf("Average: %lld\n", average);

    gpiod_line_release(interrupt);
    gpiod_line_release(led);
    gpiod_chip_close(chip);
    return 0;
}
