#ifndef STRUCTS_H
#define STRUCTS_H

#include <pthread.h>

// Vehicle implementation in a struct.
typedef struct {
    int id;
    int type;       // 1 = motorcycle, 2 = car, 3 = bus, 4 = truck
    int special;    // 0 = normal, 1 = special
    int port_id;
    int booth_id;
} Vehicle;

typedef struct Node Node;

// Linked list implementation in a struct.
struct Node {
    Vehicle* data;
    Node* next;
};

// Queue struct containing the head of the list.
typedef struct {
    Node* head;
} Queue;

// Booth implementation in a struct.
typedef struct {
    int id;
    pthread_mutex_t booth_lock;
} Booth;

// Ferry implementation in a struct.
typedef struct {
    int id;
    int port_id;
    int docked; // bool, 0 for sailing, 1 for waiting
    int waiting_amount;
    int ready_to_load;
    int ready_for_round_trip;
    Queue loading_line;
    pthread_mutex_t ferry_lock;
    pthread_mutex_t waiting_lock;
} Ferry;

// Port implementation in a struct.
typedef struct {
    int id;
    Booth booths[4];
    Queue waiting_lines[3];
    pthread_mutex_t waiting_line_lock;
    int current_line;
} Port;

// Function declarations for queue system.
void enqueue(Queue* list, Vehicle* v);
void dequeue(Queue* list);
int length(Queue* list);
void print_queue(Queue* list);
void new_queue(Queue* list);
// Function declaration for getting vehicle type as a string.
char* get_vehicle_type(Vehicle* v);

#endif
