#include <iostream>
#include <cstring>
#include "hash.hpp"

static struct Node* global_bucket_array[MAX_CAPACITY];
static struct Node node_pool[MAX_CAPACITY];
static int pool_index = 0;
//silinen node'ları kullanabilmek için çöp kutusu(free list)
static struct Node* free_list_head= NULL;


void setNode(struct Node* node, uint64_t orderID, int32_t price, uint32_t shares,  bool isBid){
    node->orderID = orderID;//64-bit değer(kopyalama yapılıyor)
    node -> price = price;//order Book adersi
    node -> shares = shares;
    node -> isBid = isBid;

    node -> next = NULL;//başlangıçta boş
    return;
}

//bucket dizisini RAM'de statik olarak ayırıyoruz

void initializeHashMap(struct hashMap* mp){
    //default capacity in this case
    mp->capacity = MAX_CAPACITY;
    mp-> numOfElements = 0;
    //statik dizinin adersini atıyoruz
    mp->arr = global_bucket_array;

    //tüm bucket'ları başlangıçta null yapıyoruz

    for(int i= 0;i< mp->capacity; i++){
        mp->arr[i] = NULL;

    }
    //pool ve free list'i sıfırla 
    pool_index = 0;
    free_list_head = NULL;

}

int hashFunction(struct hashMap* mp, const uint64_t orderID) {
    uint32_t h = (uint32_t) (orderID^(orderID >> 32));
    //basit bir asal sayı çapanıyla dağılımı ayarlanıyor
    return (int)(h & mp->capacity-1);
}

struct Node* allocateNode(){
    //önce boştaki listelere bakıp doldurmaya çalışalım 
    if(free_list_head != NULL){
        struct Node* recycleNode = free_list_head;
        free_list_head = free_list_head->next;
        return recycleNode;
    }
    //yoksa ana havuzdan bir tane ver 

    if(pool_index < MAX_CAPACITY){
        return &node_pool[pool_index++];
    }
    return NULL;//yer kalmadıysa

}

void insert(struct hashMap* mp, uint64_t orderID, int32_t price, uint32_t shares,  bool isBid){
    //getting bucket index for the given
    //key-value pair
    int bucketIndex = hashFunction(mp, orderID);
    struct Node* newNode = allocateNode();
    if(newNode != NULL){
        //içini doldurdulk
        setNode(newNode, orderID, price, shares, isBid);
        //çakışma yönetimi
        //yeni düğümün 'next'i kovadaki ilk elemanı göstersin 
        newNode ->next = mp->arr[bucketIndex];
        mp->arr[bucketIndex] = newNode;

        mp ->numOfElements++;
        return;
    }
}
void deleteKey(struct hashMap* mp, uint64_t orderID){
    //getting bucket index for the given key
    int bucketIndex = hashFunction(mp, orderID);
    struct Node* prevNode  = NULL;

    //points to the head of linked list present at 
    //bucket index

    struct Node* currNode = mp->arr[bucketIndex];
    while(currNode != NULL){
        //key is matched at delete this from linked list
        if(currNode->orderID == orderID){
            //head node 
            //deletion
            if(prevNode ==NULL) mp->arr[bucketIndex] = currNode->next;
            else prevNode ->next = currNode->next;

            //node u freelist'e ekleme
            currNode ->next = free_list_head;
            free_list_head = currNode;
            
            //güncel düğümü serbest bırakmamız gerekiyor


            mp ->numOfElements--;
            break;
        }
        prevNode = currNode;
        currNode = currNode->next;
    }
    return;
}


struct Node* search(struct hashMap* mp, uint64_t orderID){
    int bucketIndex = hashFunction(mp, orderID);
    struct Node* bucketHead = mp->arr[bucketIndex];

    while(bucketHead != NULL){
        //if key is found in the hash map
        if(bucketHead->orderID == orderID){
            return bucketHead;

        }
        bucketHead = bucketHead ->next;

    }
    //if no key found in the hash map equal to the given key
    std::cout<<"not found!!\n";
    return NULL;
}