#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "p3170127-p3170046-p3170235-pizza2.h"


pthread_mutex_t available_cooks_m;
pthread_mutex_t available_ovens_m;
pthread_mutex_t available_deliverers_m;
pthread_mutex_t delivery_total_time_m;
pthread_mutex_t gettingCold_total_time_m;
pthread_mutex_t lock;

pthread_cond_t available_cooks_cond;
pthread_cond_t available_ovens_cond;
pthread_cond_t available_deliverers_cond;

int available_cooks = N_COOKS;
int available_ovens = N_OVEN;
int available_deliverers = N_DELIVERER;
double delivery_total_time = 0;
double gettingCold_total_time = 0;
double maxDelivery_total_time = 0;
double maxGettingCold_total_time = 0;
int seed,r_time;
double total_timeD, total_timeC;


void *order(void *customer_id) {

    struct timespec start1, stop, start2;
    int rc;
    int *id = (int *)customer_id;

    
    int pizzas = rand_r(&seed) % (N_ORDERLOW + N_ORDERHIGH); //number of pizzas at each order
    int delivery = rand_r(&seed) % (T_LOW + T_HIGH); //number of minutes to deliver the pizza

    clock_gettime(CLOCK_REALTIME, &start1);
    
    //finding cooks to prepare the order
    rc = pthread_mutex_lock(&available_cooks_m);
    while (available_cooks <= 0) {
        printf("There are no cooks available to take order %d...\n\n", *id); 
        rc = pthread_cond_wait(&available_cooks_cond, &available_cooks_m);
    }
    printf("The cook is making order %d...\n\n", *id);
    --available_cooks;
    rc = pthread_mutex_unlock(&available_cooks_m);
   
    sleep(T_PREP*pizzas);


    //finding ovens to bake the pizza
    rc = pthread_mutex_lock(&available_ovens_m);
    while (available_ovens <= 0) {
        printf("There are no ovens available to bake order %d...\n\n", *id);        
        rc = pthread_cond_wait(&available_ovens_cond, &available_ovens_m);
    }
    printf("The pizzas of the order %d are being baked...\n\n", *id);
    --available_ovens;
    rc = pthread_mutex_unlock(&available_ovens_m);

    
    //The cook found an oven, so he is getting released
    rc = pthread_mutex_lock(&available_cooks_m);
    ++available_cooks;
    rc = pthread_cond_signal(&available_cooks_cond);
    rc = pthread_mutex_unlock(&available_cooks_m);

    sleep(T_BAKE);

    clock_gettime(CLOCK_REALTIME, &start2); //the pizzas have been baked, so they start getting cold


    //finding deliverer to deliver the order
    rc = pthread_mutex_lock(&available_deliverers_m);
    while (available_deliverers <= 0) {
        printf("There are no deliverers available to take order %d...\n\n", *id); 
        rc = pthread_cond_wait(&available_deliverers_cond, &available_deliverers_m);
    }
    printf("The deliverer is making order %d...\n\n", *id);
    --available_deliverers;
    rc = pthread_mutex_unlock(&available_deliverers_m);

    //The deliverer takes the pizza out of the oven, so the oven is getting released
    rc = pthread_mutex_lock(&available_ovens_m);
    printf("The pizzas of the order %d have been baked...\n\n", *id);
    ++available_ovens;
    rc = pthread_cond_signal(&available_ovens_cond);
    rc = pthread_mutex_unlock(&available_ovens_m);


    sleep(delivery); //time to deliver the pizza

    clock_gettime(CLOCK_REALTIME, &stop);

    sleep(delivery); //time for the deliverer to return at the pizzeria
    

    //the deliverer is getting released
    rc = pthread_mutex_lock(&available_deliverers_m);
    printf("The deliverer of the order %d have delivered the pizza...\n\n", *id);
    ++available_deliverers;
    rc = pthread_cond_signal(&available_deliverers_cond);
    rc = pthread_mutex_unlock(&available_deliverers_m);
   

   

    total_timeD = stop.tv_sec - start1.tv_sec;
    rc = pthread_mutex_lock(&delivery_total_time_m);
    delivery_total_time += total_timeD; //calculation of the total time of the delivery
    rc = pthread_mutex_unlock(&delivery_total_time_m);

    if (maxDelivery_total_time < total_timeD) 
    {
        maxDelivery_total_time = total_timeD;
    }

    total_timeC = stop.tv_sec - start2.tv_sec;
    rc = pthread_mutex_lock(&gettingCold_total_time_m);
    gettingCold_total_time += total_timeC; //calculation of the total time of getting cold
    rc = pthread_mutex_unlock(&gettingCold_total_time_m);

    if (maxGettingCold_total_time < total_timeC) 
    {
        maxGettingCold_total_time = total_timeC;
    }
    

    rc = pthread_mutex_lock(&lock);
    printf("Order number %d got delivered in %.2lf minutes and was getting cold for %.2lf minutes \n\n", *id, total_timeD, total_timeC);
    rc = pthread_mutex_unlock(&lock);


    pthread_exit(id);


}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("ERROR: the program should take 2 arguments. The number of customers and the seed!");
        exit(-1);
    }

    int customers = atoi(argv[1]);
    int seed = atoi(argv[2]);

    if (customers < 0) {
        printf("ERROR: The number of the customers should be a positive number!");
        exit(-1);
    }

    //initializing mutexes
    pthread_mutex_init(&available_cooks_m, NULL);
    pthread_mutex_init(&available_ovens_m, NULL);
    pthread_mutex_init(&available_deliverers_m, NULL);
    pthread_mutex_init(&delivery_total_time_m, NULL);
    pthread_mutex_init(&gettingCold_total_time_m, NULL);
    pthread_mutex_init(&lock, NULL);


    pthread_cond_init(&available_cooks_cond, NULL);
    pthread_cond_init(&available_ovens_cond, NULL);
    pthread_cond_init(&available_deliverers_cond, NULL);

    //Numbering the customers
    int customer_id[customers];
    for (int i=0; i<customers; i++) { 
        customer_id[i] = i+1;
    }

    pthread_t thread_id[customers];
    for (int i=0; i < customers; i++) {
        r_time = rand_r(&seed) % (T_ORDERLOW + T_ORDERHIGH);
        pthread_create(&thread_id[i], NULL, &order, &customer_id[i]);
		sleep(r_time);
    }

    for (int i=0; i < customers; i++) {
        pthread_join(thread_id[i], NULL);
    }

    //Destroying mutexes
    pthread_mutex_destroy(&available_cooks_m);
    pthread_mutex_destroy(&available_ovens_m);
    pthread_mutex_destroy(&available_deliverers_m);
    pthread_mutex_destroy(&delivery_total_time_m);
    pthread_mutex_destroy(&gettingCold_total_time_m);
    pthread_mutex_destroy(&lock);
    
    pthread_cond_destroy(&available_cooks_cond);
    pthread_cond_destroy(&available_ovens_cond);
    pthread_cond_destroy(&available_deliverers_cond);

    printf("The average time of the delivery is: %.2lf\n",delivery_total_time/(double)customers);
    printf("The max time of a delivery is: %.2lf\n\n", maxDelivery_total_time);

    printf("The average time of getting cold is: %.2lf\n",gettingCold_total_time/(double)customers);
    printf("The max time of getting cold is: %.2lf\n", maxGettingCold_total_time);

    return 0;
}