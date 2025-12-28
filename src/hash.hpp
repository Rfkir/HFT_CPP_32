#ifndef HASH_HPP
#define HASH_HPP
#define MAX_CAPACITY 1024
#define MAX_ORDERS 300
#include <cstring>
#include<cstdint>



struct Node{
    uint64_t orderID;
    
    //=====Patch=====
    int32_t price;
    uint32_t shares;
    bool isBid;

    struct Node* next;
};

struct hashMap{
    //Current number of elements in hashMap 
    //and capacity of hashMap
    int numOfElements;
    int capacity;
    //hold base adress of the array of linked list with static
    struct Node** arr;

};
//fonksiyon imzaları
void initializeHashMap(struct hashMap* mp);
void insert(struct hashMap* mp, uint64_t orderID, int32_t price, uint32_t shares, bool isBid);void deleteKey(struct hashMap* mp, uint64_t orderID);

struct Node* search(struct hashMap* mp, uint64_t orderID);




#endif 
