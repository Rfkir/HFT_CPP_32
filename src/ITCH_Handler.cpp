#include "hash.hpp"
#include "orderBook.hpp"
#include "HFT_ITCH.hpp"
#include <iostream>
#include <cstdint>


#define MAX_ORDERS 500
static struct orderBook myBook;//?
//takip edilmesi gereken hisseler
const char* TARGET_SYMBOLS[]={"ISCTR.E"};
const int TARGET_COUNT = 1;
static struct hashMap myMap;//tüm hisseler tek bir havuzda duracak 


using namespace HFT::ITCH;

//her bir hisse için ayrı tutulacak
struct InstrumentContext{
    uint32_t id;
    char symbol[32];//10 da olabilir?
    struct orderBook book;
    uint16_t priceDecimals;
    bool isActive;

};

//hedeflenen hisseler için yer ayırımı
static struct InstrumentContext watchList[TARGET_COUNT];

// ID verince bize ilgili hissenin Context pointer'ını döner.
// Eğer takip etmediğimiz bir ID ise NULL döner.
struct InstrumentContext* get_instrument(uint32_t id) {
    // Sayı az olduğu için döngü (Linear Search) en hızlısıdır.
    for(int i=0; i<TARGET_COUNT; ++i) {
        if(watchList[i].isActive && watchList[i].id == id) {
            return &watchList[i];
        }
    }
    return NULL; // Bizim listede yok
}

//sistemi başlatma
void init_itch_system(){
    for(int i=0; i<TARGET_COUNT; ++i) {
        watchList[i].isActive = false;
        watchList[i].book.bids = NULL;
        watchList[i].book.asks = NULL;
    }
    initializeHashMap(&myMap);
    std::cout<<"[SYSTEM] ITCH system initialized. Target number of stock: %d "<< TARGET_COUNT<<"\n";
}

//===========Order Book Directory 'R' ========================
//filtreleme burada yapılacak 

void handle_directory(const orderBookDirectory* msg){
    //gelen sembolü oku
    char incomingSymbol[32];
    std::memcpy(incomingSymbol, msg->symbol,32);
    incomingSymbol[31] = '\0';

    //gelen sembol bizim listede var mı?
    for(int i=0; i<TARGET_COUNT;i++){
        //string karşılaştırılması 'R' de yapmak yeter
        size_t len= std::strlen(TARGET_SYMBOLS[i]);
        //BIST'te semboller sola yaslı ona göre karşılaştırma yapılır

        if(std::strncmp(msg->symbol, TARGET_SYMBOLS[i], len) == 0){
            //eşleşme bulundu parametreler set ediliyor
            watchList[i].id = UTILS::swap32(msg->orderBookID);
            watchList[i].priceDecimals = UTILS::swap16(msg->numDecimalsPrice);

            //kendi listemize kaydetme işi
            std::memcpy(watchList[i].symbol, msg->symbol,32);
            watchList[i].symbol[31] = '\0';//temizlik
            watchList[i].isActive = true;

            std::cout<< "[WATCHLIST] Found: "<<watchList[i].symbol<<"(ID: "<<watchList[i].id<<")\n";
            return;//bulundu

        }
    }
}


//ADD Order 'A'
void handle_add_order(const AddOrderMsg* msg){
    //endianess dönüşümleri
    uint32_t bookID = UTILS::swap32(msg->orderBookId);

    //order ID bizim lisemizde var mı?
    struct InstrumentContext* instrument = get_instrument(msg->orderBookId);
    if(instrument == NULL) return; //bizim hisseden değil


    //buraya geldiysek bizim hissedir
    uint64_t orderID = UTILS::swap64(msg->orderId);
    int32_t price = UTILS::decodePrice(msg->rawPrice);
    uint32_t qty = (uint32_t) UTILS::swap64(msg->quantity);
    bool isBid = (msg->side == 'B');
    //order book fiyat seviyesi güncellemesi
    struct PriceLevel** targetTree = isBid ? &myBook.bids : &myBook.asks;
    
    updatePriceInBook(targetTree, price, (int32_t) qty, isBid);

    //tekil emirin güncellemesi (level3)
    insert(&myMap, orderID, price, qty, isBid);

    // Debug Log
    // std::cout << "[ADD] OID: " << oid << " Price: " << price << " Qty: " << qty << "\n";

    //hash map ekle
}


//Order Executed 'E'
void handle_executed(const MsgOrderExecuted* msg){

    uint32_t bookID = UTILS::swap32(msg->orderBookId);
    
    // Filtreleme: Bizim hisse mi?
    struct InstrumentContext* inst = get_instrument(bookID);
    if (inst == NULL) return;
    
    
    uint64_t orderID = UTILS::swap64(msg->orderId);
    uint32_t executedQty = (uint32_t) UTILS::swap64(msg->executedQty);

    //hash map'ten emri bulma
    struct Node* node = search(&myMap,orderID);
    if(node == NULL){
        //emir burda yok
        return;
    }
    //order book'tan miktarı düş(level 2 update)
    struct PriceLevel** targetTree = node->isBid ? &myBook.bids: &myBook.asks;
    reducePriceInBook(targetTree,node->price,executedQty);

    //hash map'teki emri güncelle (level 3 update)
    if(node ->shares <= executedQty){
        //emir tamamen bitti 
        deleteKey(&myMap, orderID);
    }
    else{
        //miktarı azaltma 
        node ->shares -= executedQty;
    }
}


//Order Executed 'C'
void handle_executed_price(const MsgOrderExecutedPrice* msg){
    
    uint32_t bookID = UTILS::swap32(msg->orderBookId);
    struct InstrumentContext* inst = get_instrument(bookID);
    if (inst == NULL) return;


    uint64_t orderID = UTILS::swap64(msg->orderId);
    uint32_t executedQty = (uint32_t) UTILS::swap64(msg->executedQty);

    struct Node* node = search(&myMap, orderID);
    if(!node) return;

    struct PriceLevel** targetTree = node->isBid ? &myBook.bids: &myBook.asks;
    reducePriceInBook(targetTree, node->price, node->shares);

    if (node->shares <= executedQty) {
        deleteKey(&myMap, orderID);
    } else {
        node->shares -= executedQty;
    }
}

//order delete 'D'
void handle_order_delete(const MsgOrderDelete* msg){
    
    uint32_t bookID = UTILS::swap32(msg->orderBookId);
    struct InstrumentContext* inst = get_instrument(bookID);
    if (inst == NULL) return;

    uint64_t orderID = UTILS::swap64(msg->orderId);

    //emri bul
    struct Node* node = search(&myMap, orderID);
    if(!node) return;

    //order book'tan emrin tamamını sil (level 2)
    struct PriceLevel** targetTree = node->isBid ? &myBook.bids : &myBook.asks;
    reducePriceInBook(targetTree, node->price, node->shares);

    //hash map'ten sil
    deleteKey(&myMap, orderID);

}

void handle_book_flush(const MsgOrderBookFlush* msg) {

    uint32_t bookID = UTILS::swap32(msg->orderBookId);
    struct InstrumentContext* inst = get_instrument(bookID);
    if (inst == NULL) return;

    // Bu mesaj geldiğinde hafızayı temizle.
    // Senin fonksiyonun içeriyi sıfırlıyor zaten.
    initializeHashMap(&myMap);
    
    myBook.bids = NULL;
    myBook.asks = NULL;
    
    std::cout << "[FLUSH] " << inst->symbol<< "order book flushed.\n";
}


// =============================================================
// PACKET DISPATCHER
// =============================================================
void process_buffer(const uint8_t* buffer) {
    //ilk byte mesaj tipi
    char msgType = (char)buffer[0];

    switch (msgType) {
        case 'R': handle_directory((const orderBookDirectory*)buffer); break;
        case 'A': handle_add_order((const AddOrderMsg*)buffer); break;
        case 'E': handle_executed((const MsgOrderExecuted*)buffer); break;
        case 'C': handle_executed_price((const MsgOrderExecutedPrice*)buffer); break;
        case 'D': handle_order_delete((const MsgOrderDelete*)buffer); break;
        case 'Y': handle_book_flush((const MsgOrderBookFlush*)buffer); break;
        default: break;
    }
}