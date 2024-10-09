#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <unistd.h>
#include <semaphore.h>
#include <mutex>

using namespace std;

//semaphores and one mutex
sem_t martiansS,terransS, dock;
mutex multex;

//count, so we can number the processes
int terrann =1, martiann=1;

//count for number in dock
int tc=0, twc=0, mc=0, mwc=0;  //count for num in dock and waiting.

/*
* use the  multex to get the id number for martian and terran processes.
*/
 
void martian() {
  int i,id,t;
  bool canuse = false;

  //geting an id number
  multex.lock() ;
    id = martiann;
    martiann++ ;
  multex.unlock();
  printf("martian %d:  Starting \n",id);

  while (1==1)  {

   //work for a while
    t = (int) rand() % 10; 
    printf("martian %d: is arriving in %d seconds\n",id,t);
    sleep (t);

    printf("martian %d: needs to use the dock\n",id);
    sem_wait (&dock); 
    if (tc == 0) {  //dock has no terrans, so no waiting
      mc++; 
      sem_post(&dock);  
    } else {
      mwc++;
      sem_post(&dock);
      printf("martian %d: is line  \n",id);
      sem_wait(&martiansS);//the line of martians waiting
       
      //finally can use the dock, mark the count up.
      // note must do this outside of the dock, CR or deadlock will happen
      sem_wait (&dock); 
        mc++;
      sem_post(&dock);
    }


    //use dock
    t = (int) rand() % 10; 
    printf("martian %d: is using the dock for %d seconds\n",id,t);
    sleep (t);

    sem_wait(&dock);
      mc--;
      if (mc == 0) {
        printf("martian %d:  signaling the terrans to start\n",id);
        for (i=0; i<twc; i++) 
          sem_post(&terransS);
        twc =0;
      }
    sem_post(&dock);
    printf("martian %d: is done using the dock and departing\n",id);
  } 
}

void terran() {
  int i,t,id;

  multex.lock() ;
    id = terrann;
    terrann++ ;
  multex.unlock();
  printf("terran %d:  Starting \n",id);

  //initial set to infinite for testing
  while (1==1) {

    t = (int) rand() % 10;
    printf("terran %d: is arriving for %d seconds\n",id,t);
    sleep (t);

    printf("terran %d: needs to use the dock\n",id,t);
    sem_wait (&dock); 
    if (mc == 0) {  //empty
      tc++;
      sem_post(&dock);
    }else {
      twc++;
      sem_post(&dock);
      printf("terran %d: is line  \n",id,t);
      sem_wait(&terransS);//the line of terrans waiting

      //finally can use the dock, mark the count up.
      // note must do this outside of the dock, CR or deadlock will happen
      sem_wait(&dock); 
         tc++;
      sem_post(&dock);
    }


    //use dock
    t = (int) rand() % 10; 
    printf("terran %d: is using the dock for %d seconds\n",id,t);
    sleep (t);

    //done
    sem_wait(&dock);
      tc--;
      if (tc == 0) {
        printf("terran %d: signaling the martians to start\n",id,t);
        for (i=0; i<mwc; i++)
          sem_post(&martiansS);
        mwc =0;
      }
    sem_post(&dock);
    printf("terran %d: is done with the dock and departing\n",id,t);
  } 
}

int main(int argc, char* argv[]) {
  int i;
  //defaults
  int num_m = 1; 
  int num_t = 1;

  if (argc ==3) {
    num_m = atoi( argv[1] );
    num_t = atoi( argv[2] );
  } 
  printf("m is %d, t is %d\n", num_m, num_t);

  thread martians[num_m];
  thread terrans[num_t];

  srand((unsigned)time(0));

//init semaphore line to 0, since everyone must wait
  sem_init(&martiansS,0,0);
//init semaphore line to 0, since everyone must wait
  sem_init(&terransS,0,0);
//init semaphore dock to 1, so first person can go in,  
//we could have just as easy used a mutex instead of semaphore for the "dock"
  sem_init(&dock,0,1);

// create threads 
  for (i=0; i<num_m; i++) {
     martians[i] = thread(martian);
  }
  printf ("martian threads created\n");

  for (i=0; i<num_t; i++) {
    terrans[i] = thread(terran);
  }
  printf ("all threads created\n");


//wait for threads to finish 
  for(i=0;i<num_m;i++) 
    martians[i].join();

  for(i=0;i<num_t;i++) 
    terrans[i].join();
  return 0;
}
