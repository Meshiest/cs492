#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

typedef struct product {
  int id;
  int life;
  time_t timestamp;
} product;

// function to make a new_product
product new_product();

// need to put input params into global vars
int num_prod; // number of produce threads
int num_consum; // number of consumer threads
int max_prod; // the maxium number of products to be generated
int maxq_size; // size of q that stores for both prod and con
int algo_type; // 0: first-come-first-serve, 1: round-robin
int quant; // quantum for round-robin algo
int seed; // seed for rng

product* queue; // the queue of products

// needed mutex stuff
pthread_mutex_t producer_mutex;
pthread_mutex_t consumer_mutex;
pthread_mutex_t queue_mutex;
pthread_cond_t full_queue; // condition to see if queue is full
pthread_cond_t empty_queue; // condition to see if queue is empty

// function to init all pthread stuff
void init_threads() {
  pthread_mutex_init(&producer_mutex, NULL);
  pthread_mutex_init(&consumer_mutex, NULL);
  pthread_mutex_init(&queue_mutex, NULL);
  pthread_cond_init(&full_queue, NULL);
  pthread_cond_init(&empty_queue, NULL);
}

// functions needed
void* produce(void*);
void* consume(void*);

int main(int argc, char** argv) {
  
  if(argc != 8) {
    printf("Wrong usage expected: %s <num producer threads> <num consumer threads> <total num of products> <size of queue to store products> <type of scheduling algorithim: 0 || 1> <value of qauntum seed for round robin> <seed for random num gen>\n", argv[0]);
    return -1;
  }

  // grab inputs conver to ints
  num_prod = atoi(argv[1]);
  num_consum = atoi(argv[2]);
  max_prod = atoi(argv[3]);
  maxq_size = atoi(argv[4]);
  algo_type = atoi(argv[5]);
  
  if(algo_type != 0 || algo_type != 1) {
    printf("Algo type must be 0 or 1 instead received %d\n", algo_type);
    return -1;
  }

  quant = atoi(argv[6]);
  seed = atoi(argv[7]);

  // make the queue the proper size
  queue = (product *) calloc(maxq_size, sizeof(product));
  // init all threads knowing that we made it this far
  init_threads();
  
  return 0;
}
