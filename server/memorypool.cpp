#include <cstdlib>
#include "memorypool.hpp"

void initpool(memorypool* mp, int size) {
    mp->capacity = size;
    mp->pool = (order*)malloc(sizeof(order) * size);
    mp->freelist = (int*)malloc(sizeof(int) * size);
    mp->head = 0;
    for (int i = 0; i < size; i++) {
        mp->freelist[i] = i;
    }
}

order* getorder(memorypool* mp) {
    if (mp->head >= mp->capacity) {
        return nullptr;
    }
    int index = mp->freelist[mp->head];
    mp->head++;
    return &mp->pool[index];
}

void freeorder(memorypool* mp, order* ord) {
    int index = ord - mp->pool;
    mp->head--;
    mp->freelist[mp->head] = index;
}
