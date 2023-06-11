#include <stdio.h>
#include <stdlib.h>
#include "structs.h"

// For adding a new vehicle to the end of the queue.
void enqueue(Queue* list, Vehicle* v) {
    Node* new = (Node*)malloc(sizeof(Node));
    new->data = v;
    new->next = NULL;
    if (list->head == NULL) {
        // No head, make vehicle the start.
        list->head = new;
    } else {
        // Add vehicle to the end.
        Node* current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new;
    }
}

// For removing a vehicle from the start of the queue.
void dequeue(Queue* list) {
    // Check if queue is empty.
    if (list->head == NULL) {
        printf("ERROR: Cannot dequeue from an empty list.\n");
        return;
    }
    // Check if queue has only one vehicle.
    if (list->head->next == NULL) {
        list->head = NULL;
        return;
    }
    // Assign second vehicle as first vehicle in a queue.
    list->head = list->head->next;
}

// For getting the total sum of types (units) of vehicles from a queue.
int length(Queue* list) {
    int sum = 0;
    Node* current = list->head;
    while (current != NULL) {
        sum += current->data->type;
        current = current->next;
    }
    return sum;
}

// For initializing a new queue.
void new_queue(Queue* list) {
    list = (Queue*)malloc(sizeof(Queue));
    list->head = NULL;
}

// For getting the string of the vehicle type.
char* get_vehicle_type(Vehicle* v) {
    char* name;
    switch (v->type)
    {
        case 1:
            name = "Motorcycle";
            break;
        case 2:
            name = "Car";
            break;
        case 3:
            name = "Bus";
            break;
        case 4:
            name = "Truck";
            break;
        default:
            name = "UNKNOWN";
            break;
    }
    return name;
}

// For printing queue data, debugging purposes only.
void print_queue(Queue* list) {
    Node* current = list->head;
    printf("------HEAD------\n");
    while (current != NULL) {
        printf("---------------\n");
        printf("Vehicle ID: %d\n", current->data->id);
        printf("Type: %d\n", current->data->type);
        printf("Queue Length: %d\n", length(list));
        printf("---------------\n");
        current = current->next;
    }
}
