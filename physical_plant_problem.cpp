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
sem_t coffees[5];  
sem_t working[5];
sem_t client_problems[2] /* each client has one problem semphamore which is either (1 - no fixed) or (0 - fixed)) */
sem_t available_techs; /* either 1 (3 techs are not available) or 0 (3 techs are available)*/
int available_techs = 0;
thread techs[5];
thread clients[2];  
thread receptionist;


void refill_coffee(){
    /* -- initialize coffee semaphores to 1 ("pour each tech a cup of coffee") -- */
    for (int i = 0; i < 5; i++){
        sem_init(&coffees[i], 0, 1);
    }    
}


/* -- TECHS 'break room / drink coffee' FUNCITON --
*/
void break_room(int tid){
    /* -- each tech drinks their coffee for a random amount of time */
    int drink_coffee_time = (int) rand() % 31;  
    sleep(drink_coffee_time);
    /* -- tech is done drinking coffee (set to 0) -- */
    sem_wait(&coffees[tid]);

    multex.lock();
    available_techs += 1;
    /* -- when 3 techs are available we will notify the help desk that we can fix their problem -- */
    if (available_techs == 3) {
        sem_post(&available_techs); 
        available_techs -= 3;
    }
    multex.unlock();
}


/* -- RECEPTIONISTS 'help desk' FUNCITON --
*/
void helpdesk(int tid){
    printf("Help desk is waiting for techs\n");
    /* -- wait for 3 techs to be available (set to 0)-- */
    sem_wait(&available_techs); 
}


/* -- CLIENTS 'do something' FUNCITON --
* (1) Get a random amount of time between 0 and 30 seconds that the client will "do something" for
* (2) Thread/"Clients" Sleeps/"Does Something" for the random amount of time 
* (3) Once time expires, "something breaks", and client calls help desk
*/
void do_something(int tid){
    while (true) {
        int do_something_time = (int) rand() % 31;  
        sleep(do_something_time);
        printf("Client %d has a problem!\n", tid);
        /* -- when someting breaks, the client calls the helpdesk -- */

    }
}

int main(){

    thread receptionist(helpdesk);

    /* -- initialize coffee semaphores to 1 ("pour each tech a cup of coffee") -- */
    for (int i = 0; i < 5; i++){
        sem_init(&coffees[i], 0, 1);
    }

    /* -- initialize working semaphores to 1 ("none of the techs are working") -- */
    for (int i = 0; i < 5; i++){
        sem_init(&working[i], 0, 1);
    }
    /* -- initialize working semaphores to 1 ("none of the techs are working") -- */
    for (int i = 0; i < 2; i++){
        sem_init(&client_problems[i], 0, 1);
    }

    /* -- initialize available_techs semaphore to 1 ("3 techs are not available") -- */
    sem_init(&available_techs, 0, 1);

    /* -- Let the clients do stuff... (create the client threads) -- */
    for (int i = 0; i < 2; i++){
        clients[i] = thread(do_something, i);
    }

    /* -- Let the techs sit in the break room and drink coffee... (create the tech threads) -- */
    for (int i = 0; i < 5; i++){
        techs[i] = thread(break_room, i);
    }

    return 0;
}

/*


1. Have the clients do stuff. Two client threads concurrently do someting for a random amount of time 
2. Once the random amount of time expires, the client calls the help desk.
3. The help desk thread runs concurrently for both clients

1. Have the techs drink their coffee. Each tech drinks their coffee for a random amount of time
2. When a tech is finished drinking its coffee we incrment the number of available techs by 1
3. when the number of available techs reaches 3

 */