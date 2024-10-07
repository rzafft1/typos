#include <cstdio>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <semaphore.h>
#include <unistd.h>
#include <mutex>
using namespace std;

/* -- The Dining Savages Problem (DESCRIPTION) --
* Client Process
*   ++ 2 Threads (clients)
*   -> clients sleep (they do something) for a random amount of time
*   -> when random time expires (something breaks), the client calls the help desk 
*   -> the client waits for physical plant techs to show up and fix the problem
*   -> once problem is fixed, they go back to working for a random amount of time before the next problem 
*
* Help Desk Process
*   ++ 1 Thread (receptionist)
*   -> waits for calls from client
*   -> tells physical plant to go fix problem when client calls
*
* Physical Plant Tech Process
*   ++ 5 Threads (techs)
*   ++ 5 Sephamores (1 for each tech, and its value is either 1 or 0 )
*   -> each tech drinks their coffee for a random amount of time
*   -> when techs finish their coffee, they wait for the help desk to tell them to help a client
*   -> techs can only help a client when 3 techs are done drinking coffee and are available to help
*   -> once 3 techs are available, the problem can be fixed
*   -> once the problem is fixed, the 3 techs go back to drinking coffee for a random amount of time
*/

/* -- SETUP VARIABLES --
* multex : our mutex lock for atomic actions 
* coffees : an array of 5 semaphores. 1 semaphore for each tech (its value is either 1 (the techs coffee is full) or 0 (the techs coffee is empty)
* techs : an array of 5 threads / "techs" for the physical plant tech process
* clients : an array of 2 threads / "clients"
* receptionist : a single threads for the receptionist that works at the help desk

*/
mutex multex;  
sem_t coffees[5];    // Semaphore for each tech drinking coffee
sem_t notify;        // Semaphore to notify exactly three techs about the job
sem_t call;          // Semaphore for client calls
sem_t job_complete[2]; // Semaphore for signaling when a client's job is completed
int client_call_tid = -1;  // Shared variable to store the client who made the call
int available_techs = 0;   // Number of available techs
mutex tech_count_lock;     // Mutex to protect available_techs counter
thread techs[5];
thread clients[2];  
thread receptionist;

/* -- CLIENT CALLS HELP DESK -- */
void call_helpdesk(int client_tid) {
    multex.lock();  
    client_call_tid = client_tid;
    // Set call to '1', i.e., a client made a call
    sem_post(&call); 
    multex.unlock();
}

/* -- TECHS DRINK COFFEE AND WAIT FOR JOBS -- */
void break_room(int tid) {
    while (true) {
        printf("<Tech> Tech %d entered the breakroom.\n", tid);

        // Each tech drinks their coffee for a random amount of time
        int drink_coffee_time = rand() % 31;
        sleep(drink_coffee_time);

        // Tech finishes drinking coffee
        sem_wait(&coffees[tid]);
        printf("<Tech> Tech %d finished their coffee.\n", tid);

        // Increase the number of available techs
        tech_count_lock.lock();
        available_techs++;
        printf("<Tech> %d techs are now available.\n", available_techs);
        tech_count_lock.unlock();

        // Wait for a job (once notified, this tech will work on it)
        sem_wait(&notify);

        printf("<Tech> Tech %d got a call from helpdesk and is ready to work on client %d's issue.\n", tid, client_call_tid);

        // Simulate job completion time
        int job_time = rand() % 11;
        sleep(job_time);
        
        printf("<Tech> Tech %d completed the job for client %d.\n", tid, client_call_tid);

        // Tech completes job and goes back to drinking coffee
        sem_post(&coffees[tid]);

        // After completing the job, the tech is no longer "available" until they finish coffee again
        tech_count_lock.lock();
        available_techs--;
        tech_count_lock.unlock();

        // Only notify the client after all techs finish the job
        if (available_techs == 0) {
            sem_post(&job_complete[client_call_tid]); 
        }
    }
}

/* -- RECEPTIONISTS 'HELP DESK' FUNCTION -- */
void helpdesk() {
    while (true) {
        // Wait for a call from a client
        sem_wait(&call); 

        printf("<Help Desk> The help desk got a call from client %d.\n", client_call_tid);

        // Wait until at least 3 techs are available
        tech_count_lock.lock();
        while (available_techs < 3) {
            tech_count_lock.unlock();  // Unlock while waiting to prevent deadlock
            sleep(1);  // Wait and check again after a brief delay
            tech_count_lock.lock();
        }
        printf("<Help Desk> 3 techs are available, notifying them about the job.\n");
        tech_count_lock.unlock();

        // Notify exactly 3 techs that a job is ready
        for (int i = 0; i < 3; i++) {
            sem_post(&notify);  // Signal to exactly 3 techs
        }
    }
}

/* -- CLIENTS DO SOMETHING AND CALL HELP DESK -- */
void do_something(int tid) {
    while (true) {
        int do_something_time = rand() % 31;  
        sleep(do_something_time);
        printf("<Client %d> I have a problem!\n", tid);
        call_helpdesk(tid);
        sem_wait(&job_complete[tid]);
        printf("<Client %d> My problem is fixed!\n", tid);
    }
}

int main(){

    // fill up every techs coffee (set to 1)
    for (int i = 0; i < 5; i++){
        sem_init(&coffees[i], 0, 1);
    }

    // set calls to 0, no clients have called
    sem_init(&call, 0, 0);
    // set notify to 0, no need to notify techs of jobs
    sem_init(&notify, 0, 0);

    // job is not complete
    sem_init(&job_complete[0], 0, 0);
    sem_init(&job_complete[1], 0, 0);

    receptionist = thread(helpdesk);
    for (int i = 0; i < 2; i++){
        clients[i] = thread(do_something, i);
    }
    for (int i = 0; i < 5; i++){
        techs[i] = thread(break_room, i);
    }

    clients[0].join();
    clients[1].join();
    techs[0].join();
    techs[1].join();
    techs[2].join();
    techs[3].join();
    techs[4].join();


    return 0;
}

/*


1. Have the clients do stuff. Two client threads concurrently do someting for a random amount of time 
2. Once the random amount of time expires, the client calls the help desk.
3. The help desk thread runs concurrently for both clients

1. Have the techs drink their coffee. Each tech drinks their coffee for a random amount of time
2. When a tech is finished drinking its coffee we incrment the number of available techs by 1
3. when the number of available techs reaches 3

run with : g++ physical_plant_problem.cpp -pthread -std=c++11 -o ppp
 */