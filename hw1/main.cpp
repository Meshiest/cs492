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
  time_t timestampms;
} product;

// need to put input params into global vars
int num_prod; // number of produce threads
int num_consum; // number of consumer threads
int max_prod; // the maxium number of products to be generated
int maxq_size; // size of q that stores for both prod and con
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
  p.timestampms = tv.tv_usec; // need ms too 

  return p;
}

// tracker variables
int p_id = 0, c_id = 0; // to keep track of producer/consumer ids
int numproduced, numconsumed; // keep track of number of things products and consumed
int q_size; // current q_size

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
}

// queue functions screw making an actual queue 
int push(product prod) { // pushes to queue
  pthread_mutex_lock(&queue_mutex); // lock to avoid deadlock

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

// functions needed
void* produce(void* id) {
  while(num_prod < max_prod) {
    // try to avoid a deadlock by locking mutex
    pthread_mutex_lock(&producer_mutex);

    // check if queue is full if it is we need to wait till its not
    while(q_size >= maxq_size) {
      pthread_cond_wait(&full_queue, &producer_mutex);
    }

    if(num_prod < max_prod) {
      product new_prod = new_product();
      push(new_prod); // make new product push to queue
      printf("Producer %i: Produced product %i at time", *(int*)id, new_prod.id);
      // need to print time here
      char buffer[30];
      //get product time stamp
      strftime(buffer, 30, "%T", localtime(&new_prod.timestamp));
      // print time stamp don't forget to add ms in case those matter
      printf("%s:%ld\n", buffer, new_prod.timestampms);
      numproduced++;
    }
    
    // signal consumers cause stuff in queue
    pthread_cond_signal(&empty_queue);
    pthread_mutex_unlock(&producer_mutex);
    // 100 ms sleep 
    usleep(100*100);
  }

  pthread_exit(NULL);
}

void* consume(void* id) {
  while(num_consum < max_prod) {
    // no deadlocks pls
    pthread_mutex_lock(&consumer_mutex); 

    // while nothing in queue wait for producer
    while(q_size <= 0) {
      pthread_cond_wait(&empty_queue, &consumer_mutex);
    }

    product prod = pop();

    // round robin algo 
    if(prod.life >= quant && algo_type == 1) {
      prod.life -= quant;

      // do math here for wait time and max min time
      push(prod);
    } else {
      // do other algo here
      
      printf("consumer %i: Has consumed product %i at time ", *(int*)id, prod.id);
      // print time here

      // calulate turn around time here
      
      numconsumed++;
    }

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
    fprintf(stderr, "usage: %s <num producer threads> <num consumer threads> <total num of products> <size of queue to store products> <type of scheduling algorithim: 0 || 1> <value of quantum seed for round robin> <seed for random num gen>\n", argv[0]);
    return 1;
  }

  // grab inputs conver to ints
  num_prod = atoi(argv[1]);
  num_consum = atoi(argv[2]);
  max_prod = atoi(argv[3]);
  maxq_size = atoi(argv[4]);
  algo_type = atoi(argv[5]);
  
  if(algo_type != 0 && algo_type != 1) {
    fprintf(stderr, "Algo type must be 0 or 1 instead received %d\n", algo_type);
    return 1;
  }

  quant = atoi(argv[6]);
  seed = atoi(argv[7]);

  // make the queue the proper size
  queue = (product *) calloc(maxq_size, sizeof(product));
  // init all threads knowing that we made it this far
  init_threads();

  // create proper number of threads and do something with them can do this in consume function 
  // do proper algo too 
  
  int num = num_consum + num_prod;
  int id[num];
  pthread_t thread[num];

  for(int i = 0; i < num_prod; i++) {
    id[i] = i;
    pthread_create(&thread[i], NULL, produce, &id[i]);
  }

  for(int i = 0; i < num_consum; i++) {
    id[i + num_prod] = i;
    pthread_create(&thread[i + num_prod], NULL, consume, &id[i + num_prod]);
  }

  for(int i = 0; i < num; i++) {
    pthread_join(thread[i], NULL);
  }

  // clean up the threads
  clean_threads();

  pthread_exit(0);

  // print min, max turnarounds
  // print min and max wait times
  return 0;
}
