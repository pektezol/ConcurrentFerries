# ConcurrentFerries

## Hop on Hop off Kocaeli-Yalova Ferry

It's almost summer time and a great density in maritime traffic is being expected. Your task in this project is to coordinate car ferries that travel between Eskihisar (Kocaeli) and Topçular (Yalova) ports. Ports on both sides have ferry toll booths and a single clerk is collecting payments from customers at each booth. After making the payment, a vehicle can enter the waiting area of the port. The waiting area is divided into separate waiting lines (queues) and accommodates cars, motorcycles, busses, and trucks until it gets full. Two ferries are traveling between ports. Once a ferry is full, it starts moving from one port to the other.

- Each port has four ferry toll booths (Booth1, Booth2, Booth3, and Booth4).
- The first three booths are open to all vehicles, whereas the last booth is open to the vehicles of a special group of people (i.e., veterans, seniors, or disabled people) who are eligible to receive a discount on the payment amount.
- A vehicle randomly chooses a toll booth. If a vehicle belongs to a member of the special group, any booth can be chosen.
- A toll booth clerk can communicate with a single customer at a time.
- Each vehicle of the same kind takes up a certain amount of space in the waiting area: a truck = 4 units, a bus = 3 units, a car = 2 units, and a motorcycle = 1 unit.
- Each waiting area is divided into 3 waiting lines (queues) where a line (Line1, Line2, and Line3) can hold up to 20 units.
- If a vehicle passes the toll booth, it enters the first line that is not full and large enough to accommodate the vehicle. If the waiting area is full or cannot accommodate the vehicle, the vehicle waits in the booth so that no other vehicle can initiate communication with the booth clerk.
- Once a ferry is at a port, it is loaded with the vehicles in the current line. If the ferry is not full after all vehicles in the current line are loaded (e.g., Line2), then the vehicles in subsequent lines (e.g., Line3 and then Line1) will be loaded onto the ferry. If the vehicle in the current line (e.g., Vehicle 2 in Line2) cannot fit into the remaining space on the ferry, the following vehicles in the current line (e.g., Vehicles 3, 4, ... in Line2) or the vehicles in subsequent lines (e.g., Vehicles 1, 2, 3, ... in Line3 and Vehicles 1, 2, 3, ... in Line1) will be loaded. The order of vehicles in a line and the order of lines are preserved during ferry loading. If the ferry is not full but none of the vehicles in the waiting area can fit into the remaining space on the ferry, then the ferry immediately starts moving to the other port. An empty ferry cannot leave a port.
- The operations over waiting lines are handled in a circular manner. Line1 is followed by Line2, Line2 is followed by Line3, and Line3 is followed by Line1. The current line is initially set to Line1 on both ports.
- The available space on both ferries is 30 units. There is initially one empty ferry at both ports. A ferry can either wait for 30 seconds before being loaded or wait until the waiting area is full for its first trip.
- A travel from Port A (Eskihisar) to Port B (Topçular) takes 6 seconds, whereas a travel from Port B to Port A takes 4 seconds.
- There are 8 vehicles of each kind, and their starting locations are randomly assigned to Kocaeli or Yalova. Each vehicle is randomly owned by either a regular person or a person from the special group. Each vehicle makes a roundtrip. For example, a vehicle travels from Port A to Port B, waits for at most 5 seconds before approaching a toll booth on Port B.

Consider the following scenario for a vehicle:

- At time t=0, Truck1 approaches Booth4 on Port A
- At time t=1, Truck1 enters Line1 on Port A
- At time t=5, Truck1 is loaded onto Ferry1 on Port A
- At time t=6, Ferry1 is moving to Port B
- At time t=12, Truck1 is at Port B
- At time t=14, Truck1 approaches Booth4 on Port B
- At time t=15, Truck1 enters Line2 on Port B
- At time t=27, Truck1 is loaded onto Ferry2 on Port B
- At time t=28, Ferry2 is moving to Port B
- At time t=32, Truck1 is at Port A

Consider the following scenario for two vehicles:

- At time t=25, Bus2 approaches Booth2 on Port A
- At time t=26, Car3 approaches Booth1 on Port A
- At time t=26, Bus2 enters Line1 on Port A
- At time t=27, Car3 enters Line1 on Port A
- At time t=35, Bus2 is loaded onto Ferry2 on Port A
- At time t=36, Car3 is loaded onto Ferry2 on Port A
- At time t=37, Ferry2 is moving to Port B
- At time t=43, Car3 is at Port B
- At time t=43, Bus2 is at Port B
- At time t=45, Car3 approaches Booth3 on Port B
- At time t=46, Car3 enters Line3 on Port B
- At time t=48, Bus2 approaches Booth1 on Port B
- At time t=49, Bus2 enters Line1 on Port B
- At time t=49, Car3 is loaded onto Ferry2 on Port B
- At time t=50, Bus2 is loaded onto Ferry2 on Port B
- At time t=51, Ferry2 is moving to Port A
- At time t=55, Car3 is at Port B
- At time t=55, Bus2 is at Port B

Consider the following scenario for a vehicle:

- At time t=15, Car2 approaches Booth3 on Port B
- At time t=15, Car2 is waiting in the booth since all lines on Port B are full
- .................
- At time t=25, Car2 enters Line2 on Port B
- .................

Using POSIX threads, mutex locks, and semaphores, implement a solution that coordinates the activities of vehicles and ferries. Using Pthreads, create the vehicles as separate threads and let them run. You can define other threads if you need. The access to all shared variables (e.g., waiting lines and toll booths) requires mutual exclusion between threads. Use the sleep method to make threads wait for a random period of time.

## Project Team

This project started as a COMP304 Operating Systems Term Project for the following members.

* [@pektezol](https://github.com/pektezol)
* [@bulugergerli](https://github.com/bulugergerli)