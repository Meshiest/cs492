/* I pledge my honor that I have abided by the Stevens Honor System
 * Isaac Hirschfeld, Jon Pavlik
 */
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <errno.h>

#define NUMP 5

#define lock pthread_mutex_lock
#define unlock pthread_mutex_unlock
#define signal pthread_cond_signal
#define wait pthread_cond_wait
#define mutex pthread_mutex_t
#define cond pthread_cond_t

mutex fork_mutex[NUMP];
mutex turn_mutex;
int turn;

int main()  
{
  int i;
  turn = 0;
  pthread_t diner_thread[NUMP]; 
  int dn[NUMP];
  void *diner();
  for (i=0;i < NUMP;i++) {
    pthread_mutex_init(&fork_mutex[i], NULL);
  }
  pthread_mutex_init(&turn_mutex, NULL);
  
  for (i=0;i<NUMP;i++){
    dn[i] = i;
    pthread_create(&diner_thread[i],NULL,diner,&dn[i]);
  }

  for (i=0;i<NUMP;i++)
    pthread_join(diner_thread[i],NULL);


  for (i=0;i<NUMP;i++)
    pthread_mutex_destroy(&fork_mutex[i]);
  

  pthread_exit(0);

}

void *diner(int *i)
{
  int n;
  int eating = 0;
  printf("INIT %d -\n",*i);
  n = *i;

  while (eating < 5) {
    lock(&turn_mutex);
    if(turn != n) {
      unlock(&turn_mutex);
      sleep(1);
      continue;
    }
 

    printf("WAIT %d --\n", n);
    sleep(1);
    printf("HUNG %d ---\n", n);
     

    lock(&fork_mutex[n]);
    lock(&fork_mutex[(n+1)%NUMP]);
    printf("EAT  %d ----\n", n);
    eating++;
    sleep(1);
    printf("DONE %d ----- %d/5\n", n, eating);
    unlock(&fork_mutex[n]);
    unlock(&fork_mutex[(n+1)%NUMP]);

    turn = (turn + 1) % NUMP;
    unlock(&turn_mutex);
  }
  pthread_exit(NULL);
}
