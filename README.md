# Monsoon 2021 - OS Assignment 5

> Pramod Rao B
> 2020111012

## Multithreaded server-client 

Entities simulated using threads:

1. Server
2. Client

### General working

The client program spawns multiple threads to request from the server. Each request is to modify a global dictionary. The server listens to the client requests and when obtained, pushes the client socket fd into a queue and calls one of its worker threads from the thread pool to service the request

- **Client thread**
  1. The client thread processes each request which is split into the time at which the request is supposed to be placed and the request itself
  2. After sleeping for the required amount of time, it sends the request to the server socket
  3. It then reads the channel for the servers response and prints it out
  4. The thread then exits
- **Server thread**
  1. The server program puts the client socket into a queue and signals a condition variable `requestCV`. The queue is of course locked using `qLock` before being operated on
  2. The worker thread, sleeps on the condition variable `requestCV` until the queue is empty
  3. When it finds a request to be processed, it pops it off the queue and works on the request
  4. There is a mutex lock present for each of the keys [0, 100] which is used to lock the dictionary entry and the vector entry `present` which indicates whether there is an actual value at the key
  5. The worker thread processes the request by appropriately locking and unlocking the `keysLock[k]` and reading/writing to `serverDictionary[k]`
  6. After obtaining the result of the request, it combines it with the `pthread_self()` which is the ID of the thread and the index of the client request itself and sends it back on the client fd
  7. The thread then repeats everything from step 1 infinitely

**Note**

The server program prints debug info which is solely server sided and is not exposed to the client. It also runs infinitely which is the expected behaviour for a server.

