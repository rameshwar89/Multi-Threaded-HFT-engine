#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP

#include "memorypool.hpp"

struct pricelevel {
    int price;
    order* head;
    order* tail;
};

extern pricelevel buylevels[maxprice];
extern pricelevel selllevels[maxprice];
extern int bestbuy;
extern int bestsell;

void initorderbook();
void insertorder(order* ord);
void removeorder(memorypool* mp, order* ord);
void processorder(memorypool* mp, order* ord);

#endif
