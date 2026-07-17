#include "orderbook.hpp"
#include <hiredis/hiredis.h>
#include <string>

using namespace std;

extern redisContext* redis;

void publishtrade(int isbuy, int price, int quantity) {
    char side = isbuy ? 'B' : 'S';
    string payload = string(1, side) + " " + to_string(price) + " " + to_string(quantity);
    redisReply* reply = (redisReply*)redisCommand(redis, "PUBLISH trades %s", payload.c_str());
    if (reply) freeReplyObject(reply);
}

pricelevel buylevels[maxprice];
pricelevel selllevels[maxprice];
int bestbuy = 0;
int bestsell = maxprice;

void initorderbook() {
    for (int i = 0; i < maxprice; i++) {
        buylevels[i].price = i;
        buylevels[i].head = nullptr;
        buylevels[i].tail = nullptr;
        selllevels[i].price = i;
        selllevels[i].head = nullptr;
        selllevels[i].tail = nullptr;
    }
}

void insertorder(order* ord) {
    pricelevel* level;
    if (ord->isbuy) {
        level = &buylevels[ord->price];
        if (ord->price > bestbuy) bestbuy = ord->price;
    } else {
        level = &selllevels[ord->price];
        if (ord->price < bestsell) bestsell = ord->price;
    }
    
    ord->next = nullptr;
    ord->prev = level->tail;
    
    if (level->tail != nullptr) {
        level->tail->next = ord;
    } else {
        level->head = ord;
    }
    level->tail = ord;
}

void removeorder(memorypool* mp, order* ord) {
    pricelevel* level;
    if (ord->isbuy) {
        level = &buylevels[ord->price];
    } else {
        level = &selllevels[ord->price];
    }

    if (ord->prev != nullptr) {
        ord->prev->next = ord->next;
    } else {
        level->head = ord->next;
    }

    if (ord->next != nullptr) {
        ord->next->prev = ord->prev;
    } else {
        level->tail = ord->prev;
    }
    
    freeorder(mp, ord);
}

void processorder(memorypool* mp, order* ord) {
    if (ord->isbuy) {
        while (ord->quantity > 0 && bestsell <= ord->price) {
            order* match = selllevels[bestsell].head;
            while (match != nullptr && ord->quantity > 0) {
                order* nextmatch = match->next;
                if (match->quantity <= ord->quantity) {
                    publishtrade(ord->isbuy, ord->price, match->quantity);
                    ord->quantity -= match->quantity;
                    removeorder(mp, match);
                } else {
                    publishtrade(ord->isbuy, ord->price, ord->quantity);
                    match->quantity -= ord->quantity;
                    ord->quantity = 0;
                }
                match = nextmatch;
            }
            if (selllevels[bestsell].head == nullptr) {
                bestsell++;
                if (bestsell >= maxprice) break;
            }
        }
    } else {
        while (ord->quantity > 0 && bestbuy >= ord->price) {
            order* match = buylevels[bestbuy].head;
            while (match != nullptr && ord->quantity > 0) {
                order* nextmatch = match->next;
                if (match->quantity <= ord->quantity) {
                    publishtrade(ord->isbuy, ord->price, match->quantity);
                    ord->quantity -= match->quantity;
                    removeorder(mp, match);
                } else {
                    publishtrade(ord->isbuy, ord->price, ord->quantity);
                    match->quantity -= ord->quantity;
                    ord->quantity = 0;
                }
                match = nextmatch;
            }
            if (buylevels[bestbuy].head == nullptr) {
                bestbuy--;
                if (bestbuy < 0) break;
            }
        }
    }
    
    if (ord->quantity > 0) {
        insertorder(ord);
    } else {
        freeorder(mp, ord);
    }
}
