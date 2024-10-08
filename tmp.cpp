#include <cstdio>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <semaphore.h>
#include <unistd.h>
#include <mutex>
#include <queue>   
using namespace std;

mutex multex;

sem_t coffees[5];
sem_t jobs[5];

sem_t call;
sem_t complete[2];

queue<int> client_queue;   
queue<int> tech_queue;

thread techs[5];
thread clients[2];  
thread receptionist;
sem_t sync_techs;  
int available_techs = 0;

void break_room(int tid) {
    while (true) {
        printf("<Tech> Tech %d entered the breakroom.\n", tid);
        
        // Each tech drinks coffee for a random amount of time
        int drink_coffee_time = rand() % 31;
        sleep(drink_coffee_time);
        
        // Tech is done drinking coffee
        sem_wait(&coffees[tid]);

        // Wait for a job
        sem_wait(&jobs[tid]);

        multex.lock();
        available_techs += 1;
        // start the job
        printf("<Tech> Tech %d was notified of a job. (%d/3) techs are now available. \n", tid, available_techs);
        if (available_techs <= 3){
            printf("<Tech> READY! Tech %d is ready for the job\n", tid);
            if (available_techs == 3){
                available_techs = 0;
                int client_tid = client_queue.front();
                client_queue.pop();
                printf("<Tech> UPDATE! Techs are fixing issue for client %d. \n", tid, client_tid);
                sem_post(&complete[client_tid]);
                for (int i = 0; i < 3; i++) {
                    sem_post(&sync_techs);  // Release techs after the job
                }
            }
        }

        multex.unlock();


        sem_wait(&sync_techs);
        // tech fills back up their coffee mug
        sem_post(&coffees[tid]);
       
    }
}


void helpdesk() {
    while (true) {
        // Wait for a client call
        sem_wait(&call);  

        // Send the job to all the techs
        for (int i = 0; i < 5; i++) {
            sem_post(&jobs[i]); 
        }
    }
}

void call_helpdesk(int client_tid) {
    multex.lock();
    // Push client to client queue 
    client_queue.push(client_tid);  
    // Call the help desk and request a service
    sem_post(&call);  
    multex.unlock();
}

void do_something(int tid) {
    while (true) {
        int do_something_time = rand() % 31;
        sleep(do_something_time);
        printf("\n+++++++++++++++++++\n<Client %d> I have a problem!!!\n+++++++++++++++++++\n", tid);
        call_helpdesk(tid);  // Client calls the helpdesk
        sem_wait(&complete[tid]);  // Wait for the job to be done
        printf("\n---------------\n<Client %d> My problem is fixed!\n---------------\n", tid);
    }
}

int main() {
    // Initialize semaphores for each tech's coffee
    for (int i = 0; i < 5; i++) {
        sem_init(&coffees[i], 0, 1);
    }

    // Initialize semaphores for job completion and readiness
    sem_init(&complete[0], 0, 0);
    sem_init(&complete[1], 0, 0);
    sem_init(&call, 0, 0);
    sem_init(&sync_techs, 0, 0);

    // Start helpdesk thread
    receptionist = thread(helpdesk);
    
    // Start client threads
    for (int i = 0; i < 2; i++) {
        clients[i] = thread(do_something, i);
    }
    
    // Start tech threads
    for (int i = 0; i < 5; i++) {
        techs[i] = thread(break_room, i);
    }

    // Join threads
    clients[0].join();
    clients[1].join();
    receptionist.join();
    for (int i = 0; i < 5; i++) {
        techs[i].join();
    }
}