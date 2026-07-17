#include <chrono>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include "concurrency.hpp"
#include "orderbook.hpp"
#include "memorypool.hpp"
#include <hiredis/hiredis.h>

using namespace std;
using namespace chrono;

string ringbuffer[ringcapacity];
int readindex = 0;
int writeindex = 0;
mutex ringlock;
condition_variable notempty;
condition_variable notfull;
memorypool globalmp;
int globalid = 0;
redisContext* redis = nullptr;

vector<int> active_clients;
mutex clients_lock;

void fastparse(string payload, order* ord) {
    ord->id = ++globalid;
    ord->isbuy = (payload[0] == 'B') ? 1 : 0;
    
    int i = 2;
    ord->quantity = 0;
    while (payload[i] >= '0' && payload[i] <= '9') {
        ord->quantity = ord->quantity * 10 + (payload[i] - '0');
        i++;
    }
    i++;
    ord->price = 0;
    while (payload[i] >= '0' && payload[i] <= '9') {
        ord->price = ord->price * 10 + (payload[i] - '0');
        i++;
    }
}

void consumerloop() {
    initpool(&globalmp, poolsize);
    initorderbook();
    
    redis = redisConnect("127.0.0.1", 6379);
    if (redis == nullptr || redis->err) {
        printf("Redis connection error\n");
        exit(1);
    }
    
    while (true) {
        unique_lock<mutex> lock(ringlock);
        while (readindex == writeindex) {
            notempty.wait(lock);
        }
        
        string payload = ringbuffer[readindex];
        readindex = (readindex + 1) % ringcapacity;
        
        notfull.notify_one();
        lock.unlock();
        
        order* neworder = getorder(&globalmp);
        if (neworder != nullptr) {
            fastparse(payload, neworder);
            
            auto start = high_resolution_clock::now();
            processorder(&globalmp, neworder);
            auto end = high_resolution_clock::now();
            
            auto latency = duration_cast<nanoseconds>(end - start).count();
            
            string response = "EXEC " + to_string(neworder->id) + " " + to_string(latency) + "ns\n";
            {
                lock_guard<mutex> lock(clients_lock);
                for (int fd : active_clients) {
                    send(fd, response.c_str(), response.length(), MSG_DONTWAIT);
                }
            }
        }
    }
}

void pushring(string payload) {
    unique_lock<mutex> lock(ringlock);
    while (((writeindex + 1) % ringcapacity) == readindex) {
        notfull.wait(lock);
    }
    
    ringbuffer[writeindex] = payload;
    writeindex = (writeindex + 1) % ringcapacity;
    
    notempty.notify_one();
}
