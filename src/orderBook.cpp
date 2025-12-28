#include "orderBook.hpp"
#include <cstring>
#include <stddef.h>


static struct PriceLevel price_pool[MAX_PRICE_LEVELS];
static int pricePoolIndex = 0;
static struct PriceLevel* priceFreeList = NULL;

//havuzdan yeni Pricelevel alanı ayırır
struct PriceLevel* allocatePriceLevel(){
    if(price_free_list != NULL){
        struct PriceLevel* res = price_free_list;
        price_free_list = price_free_list->next;
        res->next = NULL;
        return res;
    }
    if(pricePoolIndex < MAX_PRICE_LEVELS){
        return &price_pool[price_pool_index++];
    }
    return NULL;

}

//boşa çıkan PriceLevel'i havuza geri gönderir

void freePriceLevel(struct PriceLevel* pl){
    if(pl == NULL) return;
    pl->next = price_free_list;
    price_free_list = pl;
}

void updatePriceInBook(struct PriceLevel** head, int32_t price, int32_t shares, bool isBid){
    struct PriceLevel* prev= NULL;
    struct PriceLevel* curr = *head;
    //fiyat seviyesini bul veya eklenecek yeri tespit et 
    while(curr != NULL){
        if(curr->price == price){
            curr->totalShares += shares;
            return;
        }
        //sıralama kontrolleri
        //Bids: price< curr->price ise ilerle (büyükler üstte olsun)
        //Asks: price> curr->price ise ilerle(küçükler üstte kalsın)
        bool moveNext = isBid ? (price < curr->price) : (price > curr->price);
        if(!moveNext) break;
        prev = curr;
        curr = prev->next;
    }
    //eğer fiyat seviyesi yoksa yeni bir tane oluştur

    struct PriceLevel* newNode = allocatePriceLevel();
    if(newNode == NULL) return;

    newNode->price = price;
    newNode ->totalShares = shares;
    newNode ->next = curr;

    if(prev == NULL){
        *head = newNode; // en iyi fiyat değişti 

    }
    else prev->next = newNode;

}
void reducePriceInBook(struct PriceLevel** head, int32_t price, uint32_t shares){
    struct PriceLevel* prev = NULL;
    struct PriceLevel* curr = *head;

    while(curr != NULL){
        //ilgili fiyatı listeden bul 
        if(curr->price == price){
            //eğer kalan miktar silinecek miktardan fazlaysa sadece düşür
            if(curr->totalShares > shares){
                curr->totalShares -= shares;
            }
            else{
                //fiyat seviyesinde hiç lot kalmadıysa (veya daha az kaldıysa )
                if(prev ==NULL){
                    *head = curr->next;//listenin başı değişti 

                }
                else{
                    prev->next = curr->next; //araya bağlantı yap
                }
                //silineni free list'e geri ver 
                freePriceLevel(curr);
                
            }
            return;//döngüden çıkıyoruz
        }
        //fiyat  bulunmadıysa sonraki fiyat seviyeysine geç
        prev = curr;
        curr = curr->next;
    }
}
