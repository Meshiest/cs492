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
  time_t timestampus;
} product;

// need to put input params into global vars
int num_prod; // number of produce threads
int num_consum; // number of consumer threads
int max_prod; // the maxium number of products to be generated
int maxq_size; // size of q that stores for both prod and con
int real_size; // actual size of the queue
int algo_type; // 0: first-come-first-serve, 1: round-robin
int quant; // quantum for round-robin algo
int seed; // seed for rng

// function to make a new_product
product new_product() {
  static int pid = 0; // this way will be unique
  product p;

  // set product ID
  p.id = pid++;
  srand(seed);
  p.life = random() % 1024;

  // time stamp
  struct timeval tv;
  gettimeofday(&tv, NULL);
  p.timestamp = tv.tv_sec; // timestamp no seconds
  p.timestampus = tv.tv_usec; // need us too 

  return p;
}

// fibonacciiiii 
int fib(int num) {
  if(num <= 1) {
    return num;
  }
  
  return fib(num-1) + fib(num-2);
}

// tracker variables
int p_id = 0, c_id = 0; // to keep track of producer/consumer ids
int numproduced, numconsumed; // keep track of number of things products and consumed
int q_size; // current q_size
long long min_turn, max_turn; // turn around times
long long min_wait, max_wait; // wait times

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

void clean_threads() {
  pthread_mutex_destroy(&producer_mutex);
  pthread_mutex_destroy(&consumer_mutex);
  pthread_mutex_destroy(&queue_mutex);
  pthread_cond_destroy(&full_queue);
  pthread_cond_destroy(&empty_queue);
  free(queue);
}

// queue functions screw making an actual queue 
int push(product prod) { // pushes to queue
  pthread_mutex_lock(&queue_mutex); // lock to avoid deadlock

  if(maxq_size == 0 && q_size >= real_size) {
    real_size *= 2;
    queue = (product *) realloc(queue, sizeof(product) * real_size);
  }

  if(q_size == 0) { // nothing in queue yet
    queue[q_size++] = prod;
  } else {
    // move everything
    for(int i = q_size; i > 0; --i) {
      queue[i] = queue[i-1];
    }

    //put in new product and increase size
    queue[0] = prod;
    q_size++;
  }

  // release the lock
  pthread_mutex_unlock(&queue_mutex);
  return 0;
}

product pop() {
  pthread_mutex_lock(&queue_mutex); // mutex lock

  // get end of queue
  product prod = queue[--q_size];

  // unlock mutex
  pthread_mutex_unlock(&queue_mutex);
  
  return prod;
}

// time utility function
void calcturnaround(product prod) {
  // get new time
  struct timeval tv;
  gettimeofday(&tv, NULL);
  // get time difference between two 
  long int s = tv.tv_sec - prod.timestamp;
  long int us = tv.tv_usec - prod.timestampus;
  // add them to one time value
  us += s * 1e3;

  if(us <= 0) {
    return;
  }

  if(min_turn == 0 || us < min_turn) {
    min_turn = us;
  }
  if(us > max_turn) {
    max_turn = us;
  }
}

void calcwaittime(product p) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  long int s = tv.tv_sec - p.timestamp;
  long int us = tv.tv_usec - p.timestampus;

  us += s * 1e6; // add seconds to microseconds

  if (us <= 0)
    return;

  if (min_wait == 0 || us < min_wait)
    min_wait = us;
  if (us > max_wait)
    max_wait = us;
}

// functions needed
void* produce(void* id) {
  while(numproduced < max_prod) {
    // try to avoid a deadlock by locking mutex
    pthread_mutex_lock(&producer_mutex);

    // check if queue is full if it is we need to wait till its not
    while(maxq_size != 0 && q_size >= maxq_size) {
      pthread_cond_wait(&full_queue, &producer_mutex);
    }

    if(numproduced < max_prod) {
      product new_prod = new_product();
      push(new_prod); // make new product push to queue
      // need to print time here
      //get current time
      char buffer[30];
      struct timeval tv;
      gettimeofday(&tv, NULL);
      // grab time into buffer
      strftime(buffer, 30, "%T", localtime(&tv.tv_sec));
      // print time added with ms

      printf("[+ PRODUCE %4i ]: Produced product %-4i at time %s:%0.6ld\n", *(int*)id, new_prod.id,  buffer, tv.tv_usec);
      numproduced++;
    }
    
    // signal consumers cause stuff in queue
    pthread_cond_signal(&empty_queue);
    pthread_mutex_unlock(&producer_mutex);
    // 100 ms sleep 
    usleep(100*1000);
  }

  pthread_exit(NULL);
}

void* consume(void* id) {
  
  while(numconsumed < max_prod) {
    //printf("num_cons: %d, max: %d\n", numconsumed, max_prod);
    // no deadlocks pls
    pthread_mutex_lock(&consumer_mutex); 

    // while nothing in queue wait for producer only if we have not produced the cap
    while(q_size <= 0 && numconsumed < max_prod) {
      pthread_cond_wait(&empty_queue, &consumer_mutex);
    }

    // If we have finished producing, break out of the loop and unlock
    if(numconsumed >= max_prod) {
      pthread_mutex_unlock(&consumer_mutex);
      break;
    }

    product prod = pop();

    // round robin algo 
    if(prod.life >= quant && algo_type == 1) {
      prod.life -= quant;
      for(int i = 0; i < quant; i++) { // call fib q times
        fib(10);
      }
      // do math here for wait time and max min time
      calcwaittime(prod);
    } else {
      // do other algo herep
      for(int i = 0; i < prod.life; i++) { // call fib q times
        fib(10);
      }
      // calulate turn around time here
      calcturnaround(prod);
    }

    numconsumed++;

    char buffer[30];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // grab time into buffer
    strftime(buffer, 30, "%T", localtime(&tv.tv_sec));
    // print time added with ms
    printf("[- CONSUME %4i ]: Consumed product %-4i at time %s:%0.6ld\n", *(int*)id, prod.id, buffer, tv.tv_usec);

    // queue no longer full tell producer
    pthread_cond_signal(&full_queue);

    pthread_mutex_unlock(&consumer_mutex);
    //sleep for 100 ms
    usleep(100*100);
  }

  pthread_exit(NULL);
}

int main(int argc, char** argv) {
  
  if(argc != 8) {
    fprintf(stderr, "usage: %s <num producer threads> <num consumer threads> <total num of products> <size of queue to store products> <type of scheduling algorithim: 0 || 1> <quantum value for round robin> <seed for random num gen>\n", argv[0]);
    return 1;
  }

  // grab inputs conver to ints
  num_prod = atoi(argv[1]);
  num_consum = atoi(argv[2]);
  max_prod = atoi(argv[3]);
  maxq_size = atoi(argv[4]);
  real_size = maxq_size;

  if(maxq_size == 0)
    real_size = 8;

  algo_type = atoi(argv[5]);
  
  if(algo_type != 0 && algo_type != 1) {
    fprintf(stderr, "Algo type must be 0 or 1 instead received %d\n", algo_type);
    return 1;
  }

  quant = atoi(argv[6]);
  seed = atoi(argv[7]);

  if(quant == 0 && algo_type == 1) {
    fprintf(stderr, "Quant must be larger than 0 %d\n", quant);
    return 1;
  }

  // make the queue the proper size
  queue = (product *) calloc(real_size, sizeof(product));
  // init all threads knowing that we made it this far
  init_threads();

  // create proper number of threads and do something with them can do this in consume function 
  // do proper algo too 
  
  int num = num_consum + num_prod;
  int id[num];
  pthread_t thread[num];

  for(int i = 0; i < num_prod; i++) {
    id[i] = i;
    pthread_create(&thread[i], NULL, &produce, &id[i]);
  }

  for(int i = 0; i < num_consum; i++) {
    id[i + num_prod] = i;
    pthread_create(&thread[i + num_prod], NULL, &consume, &id[i + num_prod]);
  }

  for(int i = 0; i < num; i++) {
    pthread_join(thread[i], NULL);
  }

  // clean up the threads
  clean_threads();

  printf("\nThe minimum turnaround time is: %ld us\n", min_turn);
  printf("The maximum turnaround time is: %ld us\n", max_turn);
  printf("The minimum wait time is: %ld us\n", min_wait);
  printf("The maximum wait time is: %ld us\n", max_wait);
  
  pthread_exit(0);

  // print min, max turnarounds
  // print min and max wait times
  return 0;
}
