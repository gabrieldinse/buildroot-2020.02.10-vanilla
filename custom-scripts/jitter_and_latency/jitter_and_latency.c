#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <wiringPi.h>

#define NUM_SAMPLES 100000

unsigned int times_blink[NUM_SAMPLES];
unsigned int times_interrupt[NUM_SAMPLES];

int led_pin = 3;
int interrupt_pin = 5;

volatile int interrupt_count = 0;

sem_t sem;


void *blink_led(void *arg)
{
    int i;
    for (i = 0; i < NUM_SAMPLES; ++i)
    {
        times_blink[i] = micros();
        digitalWrite(led_pin, HIGH);
        delayMicroseconds(250);
        digitalWrite(led_pin, LOW);
        delayMicroseconds(250);
    }
}


void led_interrupt(void)
{
    times_interrupt[interrupt_count] = micros();
    interrupt_count++;
    if (interrupt_count == NUM_SAMPLES)
    {
        sem_post(&sem);
    }
}


unsigned int* calc_time_diff(unsigned int *times_blink, unsigned int *times_interrupt, int num_of_samples)
{
    int i;
    unsigned int *time_diff = (unsigned int *)malloc(NUM_SAMPLES * sizeof(unsigned int));
    for (i = 0; i < num_of_samples; ++i)
    {
        time_diff[i] = times_interrupt[i] - times_blink[i];
    }
    return time_diff;
}


unsigned int calc_average_time(int num_of_samples, unsigned int *time_diff)
{
    unsigned int average = 0;
    int i;
    for (i = 0; i < num_of_samples; ++i)
    {
        average += time_diff[i];
    }
    return average / num_of_samples;
}


void create_time_diffs_csv(char * filename, unsigned int number_of_values,
                  unsigned int *time_values){
    unsigned int n=0;
    FILE *file;
    file = fopen(filename,"w");
    while (n<number_of_values) {
       fprintf(file,"%u,%u\n",n,time_values[n]);
       n++;
    } 
    fclose(file);
}


int main(int argc, char *argv[])
{
    pthread_t task_led;
    unsigned int *time_diff;
    unsigned int average;

    if (wiringPiSetup() < 0)
    {
        fprintf(stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
        return 1;
    }
    
    pinMode(led_pin, OUTPUT);

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
    printf("Average: %u\n", average);
}
