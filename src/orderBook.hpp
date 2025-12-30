#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP
#include <stdint.h>
//#include "hash.hpp"
#define MAX_PRICE_LEVELS 500000


struct PriceLevel{
    int32_t price;
    uint32_t totalShares;
    struct PriceLevel* next;

};

struct orderBook{
    struct PriceLevel* bids;
    struct PriceLevel* asks;
};
static struct PriceLevel price_pool[MAX_PRICE_LEVELS];
static int price_pool_index = 0;
static struct PriceLevel* price_free_list = NULL;

void updatePriceInBook(struct PriceLevel** head, int32_t price, int32_t shares, bool isBid);
void reducePriceInBook(struct PriceLevel** head, int32_t price, uint32_t shares);


#endif