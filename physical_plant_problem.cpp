#include <cstdio>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <semaphore.h>
#include <unistd.h>
#include <mutex>
#include <queue>  // For queue
using namespace std;

mutex multex;  
sem_t coffees[5];  
int available_techs = 0;
sem_t notify;      // Notify techs that job is available
sem_t ready;       // Used to ensure 3 techs are ready
sem_t job_complete[2];
sem_t call;  // Used to indicate techs are done
thread techs[5];
thread clients[2];  
thread receptionist;
queue<int> client_queue;  // Queue to store client TIDs
queue<int> tech_queue;
int active_client_job = -1;  // Tracks which client's job is active

void call_helpdesk(int client_tid) {
    multex.lock();
    client_queue.push(client_tid);  // Push client's tid into the queue
    sem_post(&call);  // Notify the helpdesk about the new client
    multex.unlock();
}
string queueString(queue<int> q) {
    string output = "";
    while (!q.empty()) {
        int x = q.front(); 
        output += to_string(x);  
        q.pop();  
        if (!q.empty()) {
            output += ", ";
        }
    }
    return output;
}
void break_room(int tid) {
    while (true) {
        printf("<Tech> Tech %d entered the breakroom.\n", tid);
        
        // Each tech drinks coffee for a random amount of time
        int drink_coffee_time = rand() % 31;
        sleep(drink_coffee_time);
        
        // Tech is done drinking coffee
        sem_wait(&coffees[tid]);

        // Wait for a job
        sem_wait(&notify);

        multex.lock();
        tech_queue.push(tid);
        cout << "<Tech> " << tech_queue.size() << " techs are now available. " << "Techs in the queue are " << queueString(tech_queue) << endl;
        if (tech_queue.size() == 3){
            sem_post(&ready);
        }
        multex.unlock();
        //sem_wait(&notify);
        break;
    }
}

/* Helpdesk function */
void helpdesk() {
    while (true) {
        sem_wait(&call);  // Wait for a client call

        // Notify all techs that a client called
        for (int i = 0; i < 5; i++) {
            sem_post(&notify);
        }

        sem_wait(&ready); // wait until 3 workers are ready
        cout << "<HelpDesk> " << queueString(tech_queue) << " are working on the job for client " << endl;
    }
}

void do_something(int tid) {
    while (true) {
        int do_something_time = rand() % 31;
        sleep(do_something_time);
        printf("<Client %d> I have a problem!\n", tid);
        call_helpdesk(tid);  // Client calls the helpdesk
        sem_wait(&job_complete[tid]);  // Wait for the job to be done
        printf("<Client %d> My problem is fixed!\n", tid);
    }
}

int main() {
    // Initialize semaphores for each tech's coffee
    for (int i = 0; i < 5; i++) {
        sem_init(&coffees[i], 0, 1);
    }
    
    // Initialize semaphores for job completion and readiness
    sem_init(&notify, 0, 0);
    sem_init(&ready, 0, 0);
    sem_init(&call, 0, 0);
    sem_init(&job_complete[0], 0, 0);
    sem_init(&job_complete[1], 0, 0);

    // // Start helpdesk thread
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

/*


1. Have the clients do stuff. Two client threads concurrently do someting for a random amount of time 
2. Once the random amount of time expires, the client calls the help desk.
3. The help desk thread runs concurrently for both clients

1. Have the techs drink their coffee. Each tech drinks their coffee for a random amount of time
2. When a tech is finished drinking its coffee we incrment the number of available techs by 1
3. when the number of available techs reaches 3

run with : g++ physical_plant_problem.cpp -pthread -std=c++11 -o ppp
git add physical_plant_problem.cpp && git commit -m "Your commit message" && git push

 */
