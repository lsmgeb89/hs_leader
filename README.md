# HS Leader

## Summary
  * Utilized multi-threading and concurrent primitives ([threads][th], [mutexes][mu] & [condition variables][cv] in C++11 [Thread Support Library][tsl]) to develop a simple simulator which simulates a synchronous distributed system composed of one master thread and `n` slave threads.
  * Implemented coordinated behaviors between the master thread and `n` slave threads and [message passing][mp] in undirected links between two slave threads by using [monitors][mo] consisting of [mutexes][mu] and [condition variables][cv]
  * Implemented [HS algorithm][hs] for [leader election][le] in synchronous [ring networks][rn] under the framework of this simulator.

## Project Information
  * Course: [Distributed Computing (CS 6380)][dc]
  * Professor: [Subbarayan Venkatesan][venky]
  * Semester: Fall 2016
  * Programming Language: C++
  * Build Tool: [CMake][cmake]

## Full Requirements

### Part One: Simulator
  * You will develop a simple simulator that simulates a synchronous distributed system using multithreading.
  * There are `n + 1` processes in this synchronous distributed system: one master process and `n` slave processes. Each process will be simulated by one thread.
  * The master thread will "inform" all slave threads when one round starts.
  * Each slave threads must wait for the master thread for a "go ahead" signal before it can begin round `r`.
  * The master thread can give the signal to start round `r` only if it is sure that all the `n` slave threads have completed their previous round `r - 1`.

### Part Two: [HS Algorithm][hs]
  * Your simulation will simulate [HS algorithm][hs] for [leader election][le] in synchronous [ring networks][rn].
  * The code to implement [HS algorithm][hs] executed by all slave threads must be the same.
  * The input for this algorithm consists of two parts:
    1. `n`: the number of processes in this synchronous ring network
    2. array `id` of length `n`: `id[i]` is the unique id of the <code>i<sub>th</sub></code> process
  * The master thread reads these two inputs and then spawns `n` threads.
  * All links in the ring network are bidirectional:
    1. There is one undirected link between processes `j - 1 (mod n)` and `j`.
    2. There is one undirected link between processes `j` and `j + 1 (mod n)`.
  * No process knows the value of `n`.
  * All processes must terminate after finding the leader's id.

[th]: http://en.cppreference.com/w/cpp/thread/thread
[mu]: http://en.cppreference.com/w/cpp/thread/mutex
[cv]: http://en.cppreference.com/w/cpp/thread/condition_variable
[tsl]: http://en.cppreference.com/w/cpp/thread
[mp]: https://en.wikipedia.org/wiki/Message_passing
[mo]: https://en.wikipedia.org/wiki/Monitor_(synchronization)
[hs]: https://en.wikipedia.org/wiki/HS_algorithm
[le]: https://en.wikipedia.org/wiki/Leader_election
[rn]: https://en.wikipedia.org/wiki/Ring_network
[dc]: https://catalog.utdallas.edu/2016/graduate/courses/cs6380
[venky]: http://cs.utdallas.edu/people/faculty/venkatesan-s/
[cmake]: https://cmake.org/
