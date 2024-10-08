#include <cstdio>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <semaphore.h>
#include <unistd.h>
#include <mutex>
#include <queue>   
using namespace std;

/* -- The Physical Plant Problem (DESCRIPTION) --
* You will have three kinds of processes, client process, Help Desk process, Physical Plant Tech processes. 
* The client process will do something for a random amount of time.  Then something 
  breaks, so the client has to call physical plant to have them come out and fix it.  For the 
  client to get something fixed they must call the Help Desk.  Then wait for the Physical 
  Plant tech’s to show up to fix the problem.  Once the problem is fixed, they go back to 
  working for a random amount of time before the next problem. 
* The Help Desk has a simple job.  Wait for calls from the clients and then tell the Physical Plant Tech to go fix it. 
* Physical Plant Tech process: They are in the break room having coffee for a random mount of time.  
  When they are done with coffee, then wait for Help Desk to tell them to go out to clients.  
  But they can only go to a client’s when there are 3 tech’s (union rules) done with coffee.   
  Once there are 3 techs then the problem will get fixed, and they then tell the client the 
  problem is fixed and go back to the break room for a random amount of time having coffee. 
* Implement the actions of the processes using semaphores.  There will be 2 client threads, 1 help desk thread, and 5 Physical Plant Threads. Your code should avoid deadlock
*/

mutex multex;
sem_t coffees[5];         // 0 if coffee mug is empty, 1 if coffee mug is full (for each coffee in coffees)
sem_t problem[2];         // 0 if problem is fixed, 1 if problem is not fixed
sem_t job_notification;   // 0 if there are no jobs for techs to complete, 3 if techs need to do a job
sem_t call;               // 0 if there are no calls, 1 if there is a call from the client
sem_t working;            // 0 if no techs are working, 3 if 3 techs are working
thread techs[5];          // 5 tech threads
thread clients[2];        // 2 client threads
thread receptionist;      // 1 receptionist at the help desk
queue<int> client_queue;  // holds tids of clients who have called the help desk 
int available_techs = 0;  // the number of techs thare have accepted a job

/* -- BREAK ROOM / TECH THREADs --
* (1) Get a random amount of time between 0 and 30 seconds that the tech will "drinkn coffee" for
* (2) The tech thread sleeps/"drinks coffee" for the random amount of time 
* (3) The coffee sempahore for each tech is initialized to 1 ("their mug is full"), so when the tech is 
      done drinking coffee and sem_wait(&coffee[tid]) is executed, the value is decremented to 0 ("their mug is empty")
* (4) The tech then waits until the helpdesk sends out a job notification. Once the helpdesk sets the job_notification 
      semphamore to 3, 3 techs will decrement 'job_notification', and each tech thread that decrements it will be 1/3 of the
      techs that will go fix the problem. The remaining 2 techs that reach sem_wait(&job notification) when its value is 
      0, will have to wait until another job notification is sent out. 
* (5) Each tech that decrements job_notification (i.e. they accept the job) then must wait (with sem_wait(&working)) and 
      cannot go back to drinking coffee until the job is complete
* (6) Once 3 total techs have accepted the job, we notify the client that their problem is fixed, and the working semaphmore 
      is incremented to 3 (techs are working), so that each tech that accepted the job can decrement the working semaphore by one, 
      and once it is at 0 (no techs are working), all 3 techs can finish the job at the same time and go back to drinking coffee together.
* (7) Finally, once the techs job is complete, they will refill thier coffee mugs (set coffees[tid] to 1 with sem_post(&coffees[tid])) 
      and go back to drinking coffee. 
NOTE: use mutex locks
*/
void break_room(int tid) {
    while (true) {
        /* -- (1-2) drink coffee for random amount of time (coffees[tid] = 1) -- */
        printf("<COFFEE BREAK> Tech %d entered the breakroom.\n", tid);
        int drink_coffee_time = rand() % 31;
        sleep(drink_coffee_time);
        /* -- (3) tech is done drinking coffee, their mug is empty (coffees[tid] = 0) -- */
        sem_wait(&coffees[tid]);
        printf("<COFFEE BREAK> Tech %d has finished their coffee...\n", tid);
        /* -- (4) techs wait for a job notification (3 techs can accept the job & the remaining 2 will wait here for the next one -- */
        sem_wait(&job_notification);
        multex.lock();
        available_techs += 1;
        printf("<ALERT> Tech %d was notified of a job. (%d/3) techs are now available. \n", tid, available_techs);
        if (available_techs <= 3){
          if (available_techs == 3){
              available_techs = 0;
              int client_tid = client_queue.front();
              client_queue.pop();
              printf("<WORKING> UPDATE! Techs are fixing issue for client %d. \n", client_tid);
              /* -- (6a) notify the client that the job is complete, the client can now have more problems -- */
              sem_post(&problem[client_tid]);
              /* -- (6b) the job is complete, all techs working can go back to drinking coffee -- */
          }
          sem_post(&working);
        }
        multex.unlock();
        /* -- (5) techs that accepted the job wait until the job is complete (i.e. wait for sem_post(&working)) -- */
        sem_wait(&working);
        /* -- (7) tech refills their coffee, their mug is full (coffees[tid] = 1) -- */
        sem_post(&coffees[tid]);
       
    }
}

/* -- HELP DESK / RECEPTIONIST THREAD --
* (1) Receptionist waits for client calls with sem_wait(&call) which is initally set at 0 
* (2) Once client calls with sem_post(&call), the client notifys the techs in the breakrooom
* (3) Receptionist increments 'job_notification' sempahore to 3, so that 3 techs can be notified that there is a job to complete
*/
void helpdesk() {
    while (true) {
        /* -- wait until a client calls -- */
        sem_wait(&call);  
        /* -- notify 3 techs that there is a job to complete (unblock 3 tech threads) -- */
        for (int i = 0; i < 3; i++) {
            sem_post(&job_notification); 
        }
    }
}

/* -- CLIENT THREAD (UTILITY FUNCTION) --
* (1) Client calls function with respective tid
* (2) Clients tid is added to help desks client queue (so we can keep track of client problems)
* (3) Clients increments call semaphore to 1, notifying the help desk (who waits for a call with sem_wait(&call)) that there is a call
NOTE: use mutex locks
*/
void call_helpdesk(int client_tid) {
    multex.lock();
    client_queue.push(client_tid);  
    /* -- call the help desk (unblock receptionist thread) -- */
    sem_post(&call);  
    multex.unlock();
}

/* -- CLIENT THREADS --
* (1) Get a random amount of time between 0 and 30 seconds that the client will "do something" for
* (2) The client thread sleeps/"does something" for the random amount of time 
* (3) Once time expires, "something breaks", and client 'calls' the help desk (increment 'call' semaphore to 1)
* (4) Client thread waits for the problem to be fixed (waits while respective 'problem' semaphore = 0).
      Once respective 'problem' semaphore is set to 1 (sem_post(&problem[tid]) is called), the problem is 'fixed', 
      and the sem_wait(&problem[tid]) unblocks the client thread and decrements the value of problem[tid] back to 0 for the next problem.
*/
void do_something(int tid) {
    while (true) {
        int do_something_time = rand() % 51;
        sleep(do_something_time);
        printf("\n+++++++++++++++++++\n<Client %d> I have a problem!!!\n+++++++++++++++++++\n", tid);
        /* -- call the helpdesk to create a job for the techs -- */
        call_helpdesk(tid); 
        /* -- wait for problem to be fixed (block client thread until problem is fixed) -- */
        sem_wait(&problem[tid]); 
        printf("---------------\n<Client %d> My problem is fixed!\n---------------\n\n", tid);
    }
}

int main() {

    /* -- Initialize each techs coffee semphamore to 1 (i.e. "Their mug is full") -- */
    for (int i = 0; i < 5; i++) {
        sem_init(&coffees[i], 0, 1);
    }

    /* -- Initialize clients problem semphamore to 0 (i.e. "Their problem is not fixed" or "They dont have a problem") -- */
    for (int i = 0; i < 2; i++) {
        sem_init(&problem[i], 0, 0);
    }

    /* -- Initialize receptionists call semphamore to 0 (i.e. "No call has been received") -- */
    sem_init(&call, 0, 0);

    /* -- Initialize techs working semphamore to 0 (i.e. "No techs are working") -- */
    sem_init(&working, 0, 0);

    /* -- Start receptionist thread (i.e. "Help desk is taking client calls") -- */
    receptionist = thread(helpdesk);
    
     /* -- Start client threads (i.e. "Clients are doing / breaking things") -- */
    for (int i = 0; i < 2; i++) {
        clients[i] = thread(do_something, i);
    }
    
    /* -- Start tech threads (i.e. "Techs are in the breakroom drinking coffee") -- */
    for (int i = 0; i < 5; i++) {
        techs[i] = thread(break_room, i);
    }

    /* -- Wait for threads to complete -- */
    for (int i = 0; i < 2; i++) {
        clients[i].join();
    }
    receptionist.join();
    for (int i = 0; i < 5; i++) {
        techs[i].join();
    }
}
