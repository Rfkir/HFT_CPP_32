#ifndef HFT_OUCH_HPP
#define HFT_OUCH_HPP

#include <cstdint>
#include <cstring>


namespace HFT::OUCH{
    
    // Big Endian Helpers
    inline uint16_t swap16(uint16_t msg){return __builtin_bswap16(msg);}
    inline uint32_t swap32(uint32_t msg){return __builtin_bswap32(msg);}
    inline uint64_t swap64(uint64_t msg){return __builtin_bswap64(msg);}

    // Fast String Copier (Space Padding)
    inline void copy_str(char* dest, const char* src, size_t len){
        size_t slen=0;
        while(slen < len && src[slen] != '\0'){
            dest[slen] = src[slen]; // DÜZELTME: src[len] -> src[slen]
            slen++;
        }
        // Geri kalanını boşlukla doldur
        if (slen < len) {
            std::memset(dest + slen, ' ', len - slen);
        }
    }

    #pragma pack(push,1)
    
    // --- OUTBOUND MESSAGES (Client -> Market) ---

    // Enter Order 'O'
    struct MsgEnterOrder{
        char messageType;       // 'O'
        char token[14];         // Alpha
        uint32_t orderBookID;
        char side;
        uint64_t quantity;      // Numeric
        int32_t price;          // Price
        char timeInForce;       // 0=Day, 3=IOC, 4=FOK
        char openClose;         // 0=Default
        char clientAccount[16]; // Alpha (Offset 34)
        char customerInfo[15];  // Alpha (Offset 50)
        char exchangeInfo[32];  // Alpha (Offset 65)
        uint64_t displayQty;    // Numeric
        char clientCategory;    // Numeric
        char offHours;          // 0=Normal
        char smpLevel;          
        char smpMethod;         
        char smpId[3];          
        char reserved[2];       

        MsgEnterOrder() : messageType('O'){
            std::memset(reserved, ' ', sizeof(reserved));
        }

        // Setters
        void set_token(const char* t)    { copy_str(token, t, 14); }
        void set_book_id(uint32_t id)    { orderBookID = swap32(id); }
        void set_qty(uint64_t qty)       { quantity = swap64(qty); }
        void set_price(int32_t p)        { price = (int32_t)swap32((uint32_t)p); }
        
        // DÜZELTME: clientAccount alanına kopyalanmalı (customerInfo değil!)
        void set_client(const char* c)   { copy_str(clientAccount, c, 16); }
        
        void set_cust_info(const char* c){ copy_str(customerInfo, c, 15); }
        // DÜZELTME: İsim düzeltmesi exech -> exch
        void set_exch_info(const char* e){ copy_str(exchangeInfo, e, 32); }
    };

    // Replace Order 'U'
    struct MsgReplaceOrder{
        char messageType;        // 'U'
        char existingToken[14];  
        char newToken[14];       
        uint64_t quantity;       
        int32_t price;           
        char openClose;          
        char clientAccount[16];  
        char customerInfo[15];   
        char exchangeInfo[32];   
        uint64_t displayQty;     
        char clientCategory;     
        char reserved[8];        
        
        // DÜZELTME: 'u' -> 'U' (Büyük harf olmalı)
        MsgReplaceOrder(): messageType('U'){
            std::memset(reserved, ' ', sizeof(reserved));
        }
        void set_orig_token(const char* t){ copy_str(existingToken, t, 14); }
        void set_new_token(const char* t) { copy_str(newToken, t, 14); }
        void set_qty(uint64_t q)          { quantity = swap64(q); }
        void set_price(int32_t p)         { price = swap32(p); }
        void set_client(const char* c)    { copy_str(clientAccount, c, 16); }
    };

    // Cancel Order 'X'
    struct MsgCancelOrder{
        char messageType; // 'X'
        char token[14]; 

        MsgCancelOrder(): messageType('X'){}
        void set_token(const char* t){ copy_str(token, t, 14); }
    };

    // Cancel by Order ID 'Y'
    struct MsgCancelByOrderID{
        char messageType;
        uint32_t orderBookId;
        char side;
        uint64_t orderId; // DÜZELTME: oderId -> orderId

        MsgCancelByOrderID() : messageType('Y'){} 
        
        void set_book_id(uint32_t id) { orderBookId = swap32(id); }
        // DÜZELTME: orderBookId değil orderId set edilmeli!
        void set_order_id(uint64_t id){ orderId = swap64(id); }
    };

    // --- INBOUND MESSAGES (Market -> Client) ---

    // Order Accepted 'A'
    struct MsgAccepted{
        char messageType; // 'A'
        uint64_t timeStamp;
        char token[14];
        uint32_t orderBookID;
        char side;
        uint64_t orderID;
        uint64_t quantity;
        int32_t price;
        char timeInForce;
        char openClose;
        char clientAccount[16];
        char orderState; 
        char customerInfo[15];
        char exchangeInfo[32];
        uint64_t preTradeQty;
        uint64_t displayQty;
        char clientCategory;
        char offHours;
        char smpLevel;
        char smpMethod;
        char smpID[3];

        uint64_t get_timeStamp() const { return swap64(timeStamp); }
        uint32_t get_book_id() const   { return swap32(orderBookID); }
        uint64_t get_order_id() const  { return swap64(orderID); }
        uint64_t get_qty() const       { return swap64(quantity); }
        int32_t get_price() const      { return (int32_t)swap32((uint32_t)price); }
    };

    // Mass Quote 'Q' Header
    struct MassQuoteHeader{
        char messageType; // 'Q'
        char token[14];
        char clientCategory;
        char clientAccount[16];
        char exchangeInfo[16];
        uint16_t noQuoteEntries;
        
        void set_count(uint16_t entry) { noQuoteEntries = swap16(entry); }
    };

    // DÜZELTME: QouteEntry -> QuoteEntry
    struct QuoteEntry{
        uint32_t orderBookID;
        int32_t bidPx;
        int32_t offerPx;
        uint64_t bidSize;
        uint64_t offerSize;

        void set_values(uint32_t id, int32_t bp, int32_t op, uint64_t bs, uint64_t os){
            orderBookID = swap32(id);
            bidPx = (int32_t) swap32 ((uint32_t) bp);
            offerPx = (int32_t)swap32((uint32_t)op);
            bidSize = swap64(bs);
            offerSize = swap64(os);
        }
    };

    // Order Rejected 'J'
    struct MsgRejected{
        char messageType; // 'J'
        uint64_t timeStamp;
        char token[14];
        uint32_t rejectCode; 
        
        uint32_t get_code() const { return swap32(rejectCode); }
    };

    // Order Canceled 'C' 
    struct MsgCanceled{
        char messageType; // 'C'
        uint64_t timestamp;
        char token[14];
        uint32_t orderBookID;
        char side;
        uint64_t orderID;
        uint64_t canceledQty;
        char reason;

        uint64_t get_order_id() const     { return swap64(orderID); }
        uint64_t get_canceled_qty() const { return swap64(canceledQty); }
    };

    // Order Executed 'E'
    // DÜZELTME: msgExecuted -> MsgExecuted (Büyük harf standart)
    struct MsgExecuted{
        char messageType; // 'E'
        uint64_t timeStamp;
        char token[14];
        uint32_t orderBookID;
        uint64_t executedQty;
        int32_t price;
        uint64_t matchID;
        uint32_t comboGroupID;
        char clientCategory;
        char reserved[16];

        uint64_t get_exec_qty() const { return swap64(executedQty); }
        int32_t get_price() const     { return (int32_t)swap32((uint32_t)price); }
    };

    // System Event 'S'
    struct MsgSystemEvent{
        char messageType; // 'S' 
        // DÜZELTME: tşmeStamp -> timeStamp (Türkçe karakter olmaz)
        uint64_t timeStamp;
        char eventCode; 
    };

    #pragma pack(pop)
}
#endif