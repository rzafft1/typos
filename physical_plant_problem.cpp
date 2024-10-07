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
int available_techs = 0;
sem_t call; // 1 (no call), 0 (call from helpdesk)
thread techs[5];
thread clients[2];  
thread receptionist;
int client_call_tid = -1;


void call_helpdesk(int client_tid){
    multex.lock();  
    client_call_tid = client_tid;
    // set call to '1', i.e. a client made a call
    sem_post(&call); 
    multex.unlock();
}


void break_room(int tid){
    while (true){
        printf("<Tech> Tech %d entered the breakroom.\n", tid);
        /* -- each tech drinks their coffee for a random amount of time */
        int drink_coffee_time = (int) rand() % 31;  
        sleep(drink_coffee_time);
        /* -- tech is done drinking coffee (set to 0) -- */
        sem_wait(&coffees[tid]);
        printf("<Tech> Tech %d finished their coffee.\n", tid);
    }
}


/* -- RECEPTIONISTS 'help desk' FUNCITON --
*/
void helpdesk(){
    while (true){
        sem_wait(&call); // wait for a client to call (i.e. wait for call to be set to 1 then set it to 0)
        printf("<Help Desk> The help desk got a call from %d.\n", client_call_tid);
    }
}


void do_something(int tid){
    while (true) {
        int do_something_time = (int) rand() % 31;  
        sleep(do_something_time);
        printf("<Client %d> I have a problem!\n", tid);
        call_helpdesk();
    }
}

int main(){

    // fill up every techs coffee (set to 1)
    for (int i = 0; i < 5; i++){
        sem_init(&coffees[i], 0, 1);
    }

    // set calls to 0, no clients have called
    sem_init(&call, 0, 0);

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