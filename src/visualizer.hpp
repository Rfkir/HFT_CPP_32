#ifndef VISUALIZER_HPP
#define VISUALIZER_HPP

#include <iostream>
#include <iomanip>
#include "orderBook.hpp"//level 2 defter yapısı

inline void clear_screen(){
    std::cout<<"\033[2J\033[1;1H";

}

//defteri basma işi

inline void print_book(const struct orderBook* book, uint32_t obid){
    if(!book) return;
    // Başlık
    std::cout << "------------------------------------------------------------\n";
    std::cout << " ORDER BOOK (ID: " << obid << ") \n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "   QTY   |   BID   ||   ASK   |   QTY   \n";
    std::cout << "------------------------------------------------------------\n";

    //pointer'ları al 

    struct PriceLevel* bid = book->bids;
    struct PriceLevel* ask = book->asks;

    //ilk 10 kademeyi bastırma işi
    for(int i= 0; i<10;i++){
        if (bid) {
            std::cout << std::setw(8) << bid->totalShares << " | "
                      << std::setw(7) << bid->price << " || ";
            bid = bid->next;
        } else {
            std::cout << "         |         || ";
        }

        // Sağ Taraf (ASKS)
        if (ask) {
            std::cout << std::setw(7) << ask->price << " | "
                      << std::setw(8) << ask->totalShares;
            ask = ask->next;
        } else {
            std::cout << "         |         ";
        }
        
        std::cout << "\n";
    }

    std::cout << "------------------------------------------------------------\n";
    
}

#endif
