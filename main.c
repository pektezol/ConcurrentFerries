#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "structs.h"

#define NUM_VEHICLES 32

// Shared resources
Port ports[2];
Vehicle* vehicles[NUM_VEHICLES];
Ferry* ferries[2];
pthread_mutex_t print_lock;
// Threads
int vehicle_threads_completed[NUM_VEHICLES];
int vehicle_start_ports[NUM_VEHICLES];
int vehicle_end_ports[NUM_VEHICLES];
pthread_t vehicle_threads[NUM_VEHICLES];
pthread_t ferry_threads[2];
// Function declarations
int get_total_vehicles_in_port(int port_id);
int vehicle_left_in_booths(int port_id);
int ferry_in_port(int port_id);
int available_vehicle_left(Ferry* f);
int board_ferry(Ferry* f, Vehicle* v);
void* ferry_thread(void* arg);
void* vehicle_thread(void* arg);
void msleep(int ms);

int main() {
    // Set seed for rng.
    srand(time(NULL) % 306);
    printf("INFO: Initialization begun.\n");
    pthread_mutex_init(&print_lock, NULL);
    // Create 2 Ports.
    for (int i = 0; i < 2; i++) {
        ports[i].id = i;
        printf("INFO: Created new port with id %d.\n", ports[i].id);
        // Create 4 Booths.
        for (int j = 0; j < 4; j++) {
            ports[i].booths[j].id = j;
            int result = pthread_mutex_init(&ports[i].booths[j].booth_lock, NULL);
            if (result != 0) {
                printf("ERROR: Could not initialize mutex lock for booth.");
                exit(1);
            }
            printf("INFO: Created new booth with id %d.\n", ports[i].booths[j].id);
        }
        // Create 3 Waiting Lines.
        for (int j = 0; j < 3; j++) {
            new_queue(&ports[i].waiting_lines[j]);
            printf("INFO: Created new waiting line with id %d in port %d.\n", j, ports[i].id);
        }
        int result = pthread_mutex_init(&ports[i].waiting_line_lock, NULL);
        if (result != 0) {
            printf("ERROR: Could not initialize mutex lock for waiting line.");
            exit(1);
        }
    }
    // Create 2 Ferries.
    for (int i = 0; i < 2; i++) {
        ferries[i] = malloc(sizeof(Ferry));
        ferries[i]->id = i;
        ferries[i]->port_id = i;
        ferries[i]->docked = 1;
        int result = pthread_mutex_init(&ferries[i]->ferry_lock, NULL);
        if (result != 0) {
            printf("ERROR: Could not initialize mutex lock for ferry.");
            exit(1);
        }
        result = pthread_mutex_init(&ferries[i]->waiting_lock, NULL);
        if (result != 0) {
            printf("ERROR: Could not initialize mutex lock for ferry.");
            exit(1);
        }
        new_queue(&ferries[i]->loading_line);
        printf("INFO: Created new ferry with id %d in port %d.\n", ferries[i]->id, ports[i].id);
    }
    // Create NUM_VEHICLES Vehicles. (Default: 32)
    for (int i = 0; i < NUM_VEHICLES; i++) {
        vehicles[i] = malloc(sizeof(Vehicle));
        vehicles[i]->id = i;
        vehicles[i]->special = rand() % 2;
        vehicles[i]->port_id = rand() % 2;
        vehicles[i]->booth_id = -1;
        vehicle_start_ports[i] = vehicles[i]->port_id;
        // Assign type (unit) based on vehicle id.
        if (i < 8) {
            vehicles[i]->type = 1; // Motorcycle
        } else if (i < 16) {
            vehicles[i]->type = 2; // Car
        } else if (i < 24) {
            vehicles[i]->type = 3; // Bus
        } else {
            vehicles[i]->type = 4; // Truck
        }
    }
    printf("INFO: Initialization done.\n");
    printf("INFO: Creating threads.\n");
    // Create 2 Ferry threads with ferry_thread func and assign Ferry data from ferries[] array.
    for (int i = 0; i < 2; i++) {
        pthread_create(&ferry_threads[i], NULL, ferry_thread, ferries[i]);
    }
    // Create NUM_VEHICLES Vehicle threads with vehicle_thread func and assign Vehicle data from vehicles[] array.
    for (int i = 0; i < NUM_VEHICLES; i++) {
        pthread_create(&vehicle_threads[i], NULL, vehicle_thread, vehicles[i]);
    }
    // Join vehicle threads and wait for all of them to finish.
    for (int i = 0; i < NUM_VEHICLES; i++) {
        pthread_join(vehicle_threads[i], NULL);
    }
    printf("INFO: Vehicle threads are done. Waiting for ferries..\n");
    // Join ferry threads last and wait for both of them to finish.
    for (int i = 0; i < 2; i++) {
        pthread_join(ferry_threads[i], NULL);
    }
    printf("INFO: Ferry threads are done. Testing completeness..\n");
    // Test if every vehicle has made a round trip and came back to their starting position.
    // Prints out if the program was successful or not.
    int complete = 1;
    for (int i = 0; i < NUM_VEHICLES; i++) {
        if (vehicle_start_ports[i] != vehicle_end_ports[i]) {
            printf("INFO: Vehicle (%d) started on port %d but ended on port %d!\n", i, vehicle_start_ports[i], vehicle_end_ports[i]);
            complete = 0;
        }
    }
    if (complete) {
        printf("INFO: %d/%d checks are complete. Every vehicle has made a round trip. Success!\n", NUM_VEHICLES, NUM_VEHICLES);
    } else {
        printf("INFO: Not every vehicle has made a round trip. Fail!\n");
    }
    return 0;
}

void* ferry_thread(void* arg) {
    // Grab ferry pointer from parameter.
    Ferry* f = (Ferry*)arg;
    // For tracking how many trips has a ferry made.
    int repetition = 0;
    while (1) {
        int done;
        while (1) {
            // If all the vehicles have terminated, ferry has no reason to make any more trips.
            for (int c = 0; c < NUM_VEHICLES; c++) {
                if (vehicle_threads_completed[c] == 0) {
                    done = 0;
                    break;
                }
                done = 1;
            }
            if (done) {
                break;
            }
            sleep(1);
            pthread_mutex_lock(&f->waiting_lock);
            f->waiting_amount++;
            // For waiting either 30 seconds OR the whole waiting lines in the port to almost fill up, so the ferry can start loading vehicles.
            if (repetition == 0) {
                if (f->waiting_amount >= 30 || (length(&ports[f->port_id].waiting_lines[0]) > 16 && length(&ports[f->port_id].waiting_lines[1]) > 16 && length(&ports[f->port_id].waiting_lines[2]) > 16)) {
                    f->ready_to_load = 1;
                }
            } else {
                f->ready_to_load = 1;
            }
            pthread_mutex_unlock(&f->waiting_lock);
            // Lock the ferry and waiting lines.
            pthread_mutex_lock(&f->ferry_lock);
            pthread_mutex_lock(&ports[f->port_id].waiting_line_lock);
            // Check if ferry is full OR there are no more vehicles that ferry can pick up AND ferry is not empty.
            // OR check if current port has no vehicles AND target port has vehicles AND target port has no ferries. This is in order to rescue trapped vehicles in a port.
            if (((length(&f->loading_line) == 30 || !available_vehicle_left(f)) && length(&f->loading_line) != 0) ||
                ((get_total_vehicles_in_port(f->port_id) == 0 && get_total_vehicles_in_port((f->port_id == 0) ? 1 : 0) != 0) && (!ferry_in_port((f->port_id == 0) ? 1 : 0)))) {
                // We can move to the other port.
                pthread_mutex_unlock(&ports[f->port_id].waiting_line_lock);
                pthread_mutex_lock(&print_lock);
                printf("UPDATE: Ferry%d is moving to Port %d.\n", f->id, (f->port_id == 0) ? 1 : 0);
                pthread_mutex_unlock(&print_lock);
                pthread_mutex_lock(&f->waiting_lock);
                // Disable vehicle loading.
                f->ready_to_load = 0;
                pthread_mutex_unlock(&f->waiting_lock);
                // Reset wait counter, undock the ferry.
                f->docked = 0;
                f->waiting_amount = 0;
                // Take your time according to target port, and then change your port status.
                if (f->port_id == 0) {
                    sleep(6);
                    f->port_id = 1;
                } else if (f->port_id == 1) {
                    sleep(4);
                    f->port_id = 0;
                }
                // Change port id of every vehicle inside the ferry loading line.
                Node* current = f->loading_line.head;
                while (current != NULL) {
                    current->data->port_id = f->port_id;
                    current = current->next;
                }
                // Ferry has arrived to the new port. Dock and signal that you are ready for a round trip.
                pthread_mutex_lock(&print_lock);
                printf("UPDATE: Ferry%d arrived at Port %d.\n", f->id, f->port_id);
                pthread_mutex_unlock(&print_lock);
                f->docked = 1;
                f->ready_for_round_trip = 1;
                pthread_mutex_unlock(&f->ferry_lock);
                break;
            }
            pthread_mutex_unlock(&ports[f->port_id].waiting_line_lock);
            pthread_mutex_unlock(&f->ferry_lock);
        }
        // If all the vehicles have terminated, ferry has no reason to make any more trips.
        if (done) {
            break;
        }
        // Wait until all the vehicles have been unloaded from the ferry loading line.
        while (1) {
            msleep(100);
            pthread_mutex_lock(&f->ferry_lock);
            if (length(&f->loading_line) == 0) {
                pthread_mutex_lock(&print_lock);
                printf("UPDATE: Ferry%d is fully unloaded on Port %d.\n", f->id, f->port_id);
                pthread_mutex_unlock(&print_lock);
                pthread_mutex_unlock(&f->ferry_lock);
                break;
            }
            pthread_mutex_unlock(&f->ferry_lock);
        }
        repetition++;
    }
    pthread_exit(NULL);
}

void* vehicle_thread(void* arg) {
    // Grab vehicle pointer from parameter.
    Vehicle* v = (Vehicle*)arg;
    // Select random booth based on if you are a special passenger or not.
    v->booth_id = rand() % (v->special ? 4 : 3);
    // Try to talk to booth.
    pthread_mutex_lock(&ports[v->port_id].booths[v->booth_id].booth_lock);
    pthread_mutex_lock(&print_lock);
    printf("UPDATE: %s (%d) approaches to Booth%d on Port %d.\n", get_vehicle_type(v), v->id, ports[v->port_id].booths[v->booth_id].id, ports[v->port_id].id);
    pthread_mutex_unlock(&print_lock);
    // Try to get in a waiting line.
    int line = 0;
    while (1) {
        msleep(100);
        pthread_mutex_lock(&ports[v->port_id].waiting_line_lock);
        // If the waiting line length would not exceed 20 if you were to join in.
        if (length(&ports[v->port_id].waiting_lines[line]) + v->type <= 20) {
            // Add vehicle to the specified waiting line.
            enqueue(&ports[v->port_id].waiting_lines[line], v);
            pthread_mutex_lock(&print_lock);
            printf("UPDATE: %s (%d) enters Line%d on Port %d.\n", get_vehicle_type(v), v->id, line, ports[v->port_id].id);
            pthread_mutex_unlock(&print_lock);
            pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
            pthread_mutex_unlock(&ports[v->port_id].booths[v->booth_id].booth_lock);
            break;
        } else {
            // This line is full, check other lines in a circular manner.
            // pthread_mutex_lock(&print_lock);
            // printf("UPDATE: %s (%d) is waiting in the booth since Line%d is full on Port %d.\n", get_vehicle_type(v), v->id, line, ports[v->port_id].id);
            // pthread_mutex_unlock(&print_lock);
            pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
            line = (line + 1) % 3;
        }
    }
    int ferry_id;
    // Try to board the ferry.
    while (1) {
        msleep(100);
        int select = 0;
        while (!select) {
            // Check all ferries: if the ferry port id is the same as vehicle port id
            // AND the ferry is docked, that's the ferry vehicle is going to board.
            for (int f = 0; f < 2; f++) {
                pthread_mutex_lock(&ferries[f]->ferry_lock);
                if (ferries[f]->port_id == v->port_id && ferries[f]->docked) {
                    ferry_id = f;
                    pthread_mutex_unlock(&ferries[f]->ferry_lock);
                    select = 1;
                    break;
                }
                pthread_mutex_unlock(&ferries[f]->ferry_lock);
            }
        }
        Node* current = ports[v->port_id].waiting_lines[ports[v->port_id].current_line].head;
        pthread_mutex_lock(&ferries[ferry_id]->ferry_lock);
        pthread_mutex_lock(&ports[v->port_id].waiting_line_lock);
        pthread_mutex_lock(&ferries[ferry_id]->waiting_lock);
        // Check the current line's head vehicle and try to board it to the ferry
        if (current != NULL && ferries[ferry_id]->docked && current->data->id == v->id && ferries[ferry_id]->ready_to_load) {
            int boarded = board_ferry(ferries[ferry_id], v);
            pthread_mutex_unlock(&ferries[ferry_id]->waiting_lock);
            // Vehicle could not board due to space, go to next line in a circular manner.
            if (!boarded) {
                ports[v->port_id].current_line = (ports[v->port_id].current_line + 1) % 3;
                pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
                pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
                continue;
            }
            // Vehicle successfully boarded.
            pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
            pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
            break;
        }
        // If the waiting line is empty, go to next line in a circular manner.
        if (current == NULL) {
            ports[v->port_id].current_line = (ports[v->port_id].current_line + 1) % 3;
        }
        pthread_mutex_unlock(&ferries[ferry_id]->waiting_lock);
        pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
        pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
    }
    // Wait until docks back.
    while (1) {
        pthread_mutex_lock(&ferries[ferry_id]->ferry_lock);
        if (ferries[ferry_id]->docked) {
            pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
            break;
        }
        pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
    }
    // Try to unload vehicle in new port.
    while (1) {
        msleep(100);
        pthread_mutex_lock(&ferries[ferry_id]->ferry_lock);
        pthread_mutex_lock(&ferries[ferry_id]->waiting_lock);
        // Check the loading line's head vehicle and try to unload it from the ferry.
        if (ferries[ferry_id]->loading_line.head != NULL && ferries[ferry_id]->ready_for_round_trip && ferries[ferry_id]->loading_line.head->data->id == v->id && !ferries[ferry_id]->ready_to_load) {
            // Unload from the start of the queue.
            pthread_mutex_unlock(&ferries[ferry_id]->waiting_lock);
            pthread_mutex_lock(&print_lock);
            printf("UPDATE: %s (%d) unloaded on Port %d.\n", get_vehicle_type(v), v->id, ports[v->port_id].id);
            pthread_mutex_unlock(&print_lock);
            // Reset vehicle's booth id.
            v->booth_id = -1;
            dequeue(&ferries[ferry_id]->loading_line);
            pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
            break;
        }
        pthread_mutex_unlock(&ferries[ferry_id]->waiting_lock);
        pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
    }
    // Start again.
    sleep((rand() % 5) + 1);
    // Select random booth based on if you are a special passenger or not.
    v->booth_id = rand() % (v->special ? 4 : 3);
    // Try to talk to booth.
    pthread_mutex_lock(&ports[v->port_id].booths[v->booth_id].booth_lock);
    pthread_mutex_lock(&print_lock);
    printf("UPDATE: %s (%d) approaches to Booth%d on Port %d.\n", get_vehicle_type(v), v->id, ports[v->port_id].booths[v->booth_id].id, ports[v->port_id].id);
    pthread_mutex_unlock(&print_lock);
    // Try to get in a waiting line.
    line = 0;
    while (1) {
        msleep(100);
        pthread_mutex_lock(&ports[v->port_id].waiting_line_lock);
        // If the waiting line length would not exceed 20 if you were to join in.
        if (length(&ports[v->port_id].waiting_lines[line]) + v->type <= 20) {
            // Add vehicle to the specified waiting line.
            enqueue(&ports[v->port_id].waiting_lines[line], v);
            pthread_mutex_lock(&print_lock);
            printf("UPDATE: %s (%d) enters Line%d on Port %d.\n", get_vehicle_type(v), v->id, line, ports[v->port_id].id);
            pthread_mutex_unlock(&print_lock);
            pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
            pthread_mutex_unlock(&ports[v->port_id].booths[v->booth_id].booth_lock);
            break;
        } else {
            // This line is full, check other lines in a circular manner.
            // pthread_mutex_lock(&print_lock);
            // printf("UPDATE: %s (%d) is waiting in the booth since Line%d is full on Port %d.\n", get_vehicle_type(v), v->id, line, ports[v->port_id].id);
            // pthread_mutex_unlock(&print_lock);
            pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
            line = (line + 1) % 3;
        }
    }
    // Try to board the ferry.
    while (1) {
        msleep(100);
        int select = 0;
        while (!select) {
            // Check all ferries: if the ferry port id is the same as vehicle port id
            // AND the ferry is docked, that's the ferry vehicle is going to board.
            for (int f = 0; f < 2; f++) {
                pthread_mutex_lock(&ferries[f]->ferry_lock);
                if (ferries[f]->port_id == v->port_id && ferries[f]->docked) {
                    ferry_id = f;
                    pthread_mutex_unlock(&ferries[f]->ferry_lock);
                    select = 1;
                    break;
                }
                pthread_mutex_unlock(&ferries[f]->ferry_lock);
            }
        }
        Node* current = ports[v->port_id].waiting_lines[ports[v->port_id].current_line].head;
        pthread_mutex_lock(&ferries[ferry_id]->ferry_lock);
        pthread_mutex_lock(&ports[v->port_id].waiting_line_lock);
        pthread_mutex_lock(&ferries[ferry_id]->waiting_lock);
        // Check the current line's head vehicle and try to board it to the ferry
        if (current != NULL && ferries[ferry_id]->docked && current->data->id == v->id && ferries[ferry_id]->ready_to_load) {
            int boarded = board_ferry(ferries[ferry_id], v);
            pthread_mutex_unlock(&ferries[ferry_id]->waiting_lock);
            // Vehicle could not board due to space, go to next line in a circular manner.
            if (!boarded) {
                ports[v->port_id].current_line = (ports[v->port_id].current_line + 1) % 3;
                pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
                pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
                continue;
            }
            // Vehicle successfully boarded.
            pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
            pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
            break;
        }
        // If the waiting line is empty, go to next line in a circular manner.
        if (current == NULL) {
            ports[v->port_id].current_line = (ports[v->port_id].current_line + 1) % 3;
        }
        pthread_mutex_unlock(&ferries[ferry_id]->waiting_lock);
        pthread_mutex_unlock(&ports[v->port_id].waiting_line_lock);
        pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
    }
    // Wait until docks back.
    while (1) {
        pthread_mutex_lock(&ferries[ferry_id]->ferry_lock);
        if (ferries[ferry_id]->docked) {
            pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
            break;
        }
        pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
    }
    // Try to unload vehicle in new port.
    while (1) {
        msleep(100);
        pthread_mutex_lock(&ferries[ferry_id]->ferry_lock);
        pthread_mutex_lock(&ferries[ferry_id]->waiting_lock);
        // Check the loading line's head vehicle and try to unload it from the ferry
        if (ferries[ferry_id]->loading_line.head != NULL && ferries[ferry_id]->ready_for_round_trip && ferries[ferry_id]->loading_line.head->data->id == v->id && !ferries[ferry_id]->ready_to_load) {
            // Unload from the start of the queue.
            pthread_mutex_unlock(&ferries[ferry_id]->waiting_lock);
            pthread_mutex_lock(&print_lock);
            printf("UPDATE: %s (%d) unloaded on Port %d.\n", get_vehicle_type(v), v->id, ports[v->port_id].id);
            pthread_mutex_unlock(&print_lock);
            // Reset vehicle's booth id.
            v->booth_id = -1;
            dequeue(&ferries[ferry_id]->loading_line);
            pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
            break;
        }
        pthread_mutex_unlock(&ferries[ferry_id]->waiting_lock);
        pthread_mutex_unlock(&ferries[ferry_id]->ferry_lock);
    }
    vehicle_threads_completed[v->id] = 1;
    vehicle_end_ports[v->id] = v->port_id;
    pthread_exit(NULL);
}

int board_ferry(Ferry* f, Vehicle* v) {
    if (v->type <= 30 - length(&f->loading_line)) {
        // We can board since there is enough space.
        pthread_mutex_lock(&print_lock);
        printf("UPDATE: %s (%d) is loaded to Ferry%d on Port %d.\n", get_vehicle_type(v), v->id, f->id, ports[v->port_id].id);
        pthread_mutex_unlock(&print_lock);
        // Add to the ferry loading line and remove from waiting line.
        enqueue(&f->loading_line, v);
        dequeue(&ports[v->port_id].waiting_lines[ports[v->port_id].current_line]);
        return 1;
    }
    return 0;
}

int available_vehicle_left(Ferry* f) {
    // Check every waiting line to see if there are any vehicle in them or not.
    for (int i = 0; i < 3; i++) {
        msleep(100);
        if (ports[f->port_id].waiting_lines[i].head != NULL && ports[f->port_id].waiting_lines[i].head->data->type <= 30 - length(&f->loading_line) && !vehicle_left_in_booths(f->port_id)) {
            return 1;
        }
    }
    return 0;
}

int get_total_vehicles_in_port(int port_id) {
    // Count the total amount of vehicles inside a specified port.
    int sum = 0;
    for (int i = 0; i < NUM_VEHICLES; i++) {
        if (vehicles[i]->port_id == port_id && vehicle_threads_completed[i] == 0) {
            sum++;
        }
    }
    return sum;
}

int vehicle_left_in_booths(int port_id) {
    // Check if there is any vehicle in the booths section.
    for (int i = 0; i < NUM_VEHICLES; i++) {
        if (vehicles[i]->port_id == port_id && vehicles[i]->booth_id == -1) {
            return 1;
        }
    }
    return 0;
}

int ferry_in_port(int port_id) {
    // Check if there is any ferry in a specified port.
    for (int i = 0; i < 2; i++) {
        if (ferries[i]->port_id == port_id && ferries[i]->docked) {
            return 1;
        }
    }
    return 0;
}

void msleep(int ms) {
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = ms * 1000000;
    nanosleep(&time, NULL);
}
