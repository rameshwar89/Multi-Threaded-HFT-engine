#ifndef CONCURRENCY_HPP
#define CONCURRENCY_HPP

#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>

using namespace std;

const int ringcapacity = 1024;
const int maxclients = 64;

extern string ringbuffer[ringcapacity];
extern int readindex;
extern int writeindex;
extern mutex ringlock;
extern condition_variable notempty;
extern condition_variable notfull;

extern vector<int> active_clients;
extern mutex clients_lock;

void consumerloop();
void pushring(string payload);

#endif
