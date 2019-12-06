#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
#include <random>
#include <ctime>
#include <set>
#include <chrono>
#include <thread>
#include <algorithm>


using namespace std;


typedef struct { //Struct for our Clients
    int id;
    int seat;
    int done;
    pthread_t thread_id;
} Client;



int NUM_SEATS; //Gloal seat number var from the argument
int* seats;
set<int> myset; //Set for selecting the remaining avaliable seats
pthread_mutex_t random_mutex = PTHREAD_MUTEX_INITIALIZER;  //Thread Mutexes and conditions
pthread_mutex_t* mutexes;
pthread_mutex_t* seat_mutexes;
pthread_cond_t* request_conditions;
pthread_cond_t* response_conditions;

FILE *fp; //Outstream for output txt



uint randomGenerator(uint min, uint max) { //Function for randomly selecting the remaining avaliable seats
    mt19937 rng;
    rng.seed(std::random_device()());
    //rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    uniform_int_distribution<std::mt19937::result_type> dist(min, max);

    return dist(rng);
}


void *server_thread(void* t) { //Server Thread Class for giving the Client seat and erasing the taken seat
    Client* client = (Client*)t;

    pthread_mutex_lock( &mutexes[client->id] );
    while(!client->done) {  // While for the client to ask for seats until it gets one
        //printf("server before wait client: %d with seat: %d\n", client->id, client->seat);
        pthread_cond_wait(&request_conditions[client->id], &mutexes[client->id]);
        //printf("signal received: %d with seat: %d seats_status: %d\n", client->id, client->seat, seats[client->seat]);

        if (seats[client->seat] == -1 && pthread_mutex_trylock(&seat_mutexes[client->seat]) == 0) { //If for asking and getting a seat
            //printf("try lock: %d with seat: %d\n", client->id, client->seat);
            seats[client->seat] = client->id;
            client->done = true;
            myset.erase(client->seat);
            pthread_mutex_unlock(&seat_mutexes[client->seat]);
        } else { //Else if its not successful
            client->seat = -1;
            client->done = false;
        }
        //printf("before signal send: %d\n", client->id);
        pthread_mutex_unlock( &mutexes[client->id] );
        pthread_cond_signal(&response_conditions[client->id]); //Signal for Client thread
        //printf("after signal send: %d with seat: %d\n", client->id, client->seat);
    }

    //printf("Client: %d exit with seat: %d\n", client->id, client->seat);
    pthread_exit(NULL);

}

void *client_thread(void* t) { //Client thread
    Client* client = (Client*)t;
    pthread_t server_tid;
    pthread_create(&server_tid, NULL, &server_thread, client);
    //sleep(randomGenerator(2, 5));
    this_thread::sleep_for(std::chrono::milliseconds(randomGenerator(50, 200))); // Waiting time


    pthread_mutex_lock( &mutexes[client->id] );
    while(!client->done) {
        //printf("top of while, client: %d\n", client->id);
        pthread_mutex_lock( &random_mutex );
        //for(set<int>::iterator iter = myset.begin(); iter != myset.end(); iter++) {
        //    printf("%d ", *iter);
        //}
        //printf("\n");

        int seat_number = -1;
        int seat_idx = NUM_SEATS+100;
        while(seat_number < 0 || seat_number > NUM_SEATS || seat_idx > myset.size()-1) { //Selection If for random seats
            set<int>::const_iterator it(myset.begin());
            seat_idx = randomGenerator(0, myset.size() - 1);
    mt19937 rng;
            advance(it, seat_idx);
            seat_number = *it;
        }
        //printf("seat number: %d seat index: %d\n", seat_number, seat_idx);
        client->seat = seat_number;
        pthread_mutex_unlock( &random_mutex );
        //printf("Client: %d seat: %d\n", client->id, client->seat);
        pthread_mutex_unlock(&mutexes[client->id]);
        pthread_cond_signal(&request_conditions[client->id]);
        //printf("before wait client: %d with seat: %d and done: %d\n", client->id, client->seat, client->done);
        pthread_cond_wait(&response_conditions[client->id], &mutexes[client->id]);
        //printf("after wait client: %d with seat: %d and done: %d\n", client->id, client->seat, client->done);
    }
    //printf("before unlock client: %d\n", client->id);
    pthread_mutex_unlock( &mutexes[client->id] );
    printf("Client%d reserves Seat%d\n", client->id+1, client->seat+1);  //Writing the reservations
    fprintf(fp, "Client%d reserves Seat%d\n", client->id+1, client->seat+1);

    pthread_exit(NULL);
}





int main (int argc, char* argv[]) { //Main
    srand((unsigned)time(0));
    fp = fopen("output.txt", "w");

    if(argc != 2) {
        fprintf(fp, "\nNo Extra Command Line Argument Passed Other Than Program Name");
        exit(EXIT_FAILURE);
    }

    string arg = argv[1];
    try {
        size_t pos;
        NUM_SEATS = stoi(arg, &pos);
        if (pos < arg.size()) {
            cerr << "Trailing characters after number: " << arg << '\n';
        }
    } catch (invalid_argument const &ex) {
        cerr << "Invalid number: " << arg << '\n';
    } catch (std::out_of_range const &ex) {
        cerr << "Number out of range: " << arg << '\n';
    }

    printf("Number of total seats: %d\n", NUM_SEATS);
    fprintf(fp, "Number of total seats: %d\n", NUM_SEATS);

    int rc;
    //pthread_t threads[NUM_SEATS];
    pthread_attr_t attr;


    pthread_attr_init(&attr);  //Initialize and set thread detached attribute
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);



    vector<Client> clients(NUM_SEATS);
    mutexes = new pthread_mutex_t[NUM_SEATS]; //Creation of the Mutexes
    seat_mutexes = new pthread_mutex_t[NUM_SEATS];
    request_conditions = new pthread_cond_t[NUM_SEATS];
    response_conditions = new pthread_cond_t[NUM_SEATS];
    seats = new int[NUM_SEATS];


    for(int i=0;i<NUM_SEATS; i++) { //Inserting the seats to the myset
        myset.insert(i);
    }



    for (int i = 0; i < NUM_SEATS; i++) {  // Initializing the basic clients and mutexes
        mutexes[i] = PTHREAD_MUTEX_INITIALIZER;
        seat_mutexes[i] = PTHREAD_MUTEX_INITIALIZER;
        request_conditions[i] = PTHREAD_COND_INITIALIZER;
        response_conditions[i] = PTHREAD_COND_INITIALIZER;
        seats[i] = -1;
        clients[i].id = i;
        clients[i].seat = -1;
        clients[i].done = false;
    }



    vector<pthread_t> threads(NUM_SEATS);
    for (int i=0;i<NUM_SEATS;++i) {
        pthread_create(threads.data() + i, &attr, &client_thread, clients.data() + i);
        //cout << "Thread Created with ID : " << i << endl;
    }



    pthread_attr_destroy(&attr); // Destroying the threads


    for_each(threads.begin(), threads.end(),
             [](pthread_t& t) { pthread_join(t, NULL); });



    printf("All seats are reserved.\n");
    fprintf(fp, "All seats are reserved.\n");

    fclose(fp);

    exit(EXIT_SUCCESS);


}

