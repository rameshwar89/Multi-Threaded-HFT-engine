#ifndef MEMORYPOOL_HPP
#define MEMORYPOOL_HPP

const int maxprice = 100000;
const int poolsize = 1000000;

struct order {
    int id;
    int price;
    int quantity;
    int isbuy;
    order* next;
    order* prev;
};

struct memorypool {
    order* pool;
    int* freelist;
    int head;
    int capacity;
};

void initpool(memorypool* mp, int size);
order* getorder(memorypool* mp);
void freeorder(memorypool* mp, order* ord);

#endif
