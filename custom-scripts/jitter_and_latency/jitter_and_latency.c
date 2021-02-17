#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <wiringPi.h>

#define NUM_SAMPLES 100000

intmax_t times_blink[NUM_SAMPLES];
intmax_t times_interrupt[NUM_SAMPLES];
struct timespec ts1;
struct timespec ts2;

int led_pin = 3; // gpio pin 22
int interrupt_pin = 5; // gpio pin 24

volatile int interrupt_count = 0;

sem_t sem;


void *blink_led(void *arg)
{
    int i;
    for (i = 0; i < NUM_SAMPLES; ++i)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        times_blink[i] = ts1.tv_sec * 1e9 + ts1.tv_nsec;
        digitalWrite(led_pin, HIGH);
        delayMicroseconds(250);
        digitalWrite(led_pin, LOW);
        delayMicroseconds(250);
    }
}


void led_interrupt(void)
{
    clock_gettime(CLOCK_MONOTONIC, &ts2);
    times_interrupt[interrupt_count] = ts2.tv_sec * 1e9 + ts2.tv_nsec;
    interrupt_count++;
    if (interrupt_count == NUM_SAMPLES)
    {
        sem_post(&sem);
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


int main(int argc, char *argv[])
{
    pthread_t task_led;
    intmax_t *time_diff;
    intmax_t average;

    if (wiringPiSetup() < 0)
    {
        fprintf(stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
        return 1;
    }
    
    pinMode(led_pin, OUTPUT);
    digitalWrite(led_pin, LOW);

    if (wiringPiISR(interrupt_pin, INT_EDGE_RISING, &led_interrupt))
    {
        fprintf(stderr, "Unable to setup ISR: %s\n", strerror (errno));
        return 1;
    }

    sem_init(&sem, 0, 0);

    pthread_create(&task_led, NULL, blink_led, NULL);
    printf("Started blinking led...\n");
    pthread_join(task_led, NULL);
    sem_wait(&sem);

    printf("Calculating time differences...\n");
    time_diff = calc_time_diff(times_blink, times_interrupt, NUM_SAMPLES);
    printf("Writing time differentes to csv...\n");
    create_time_diffs_csv("/root/time_diff.csv", NUM_SAMPLES, time_diff);
    printf("Calculating average...\n");
    average = calc_average_time(NUM_SAMPLES, time_diff);
    free(time_diff);
    printf("Average: %lld\n", average);
}
