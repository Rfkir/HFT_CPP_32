#pragma once
#include <cstdint>
#include <cstring>

namespace HFT::ITCH {
    struct InternalOrder{
        uint64_t orderID;//offset 5
        int32_t price;//decodePrice() yapılmış hali
        uint64_t rankingTime;//offset 37
        uint32_t rankingSeqNum;//offset 18
        uint64_t quantity;//offset 22
        char side;//'B' veya 'S' 
    };

    #pragma pack(push, 1)
    struct AddOrderMsg{
        char messageType ; //Offset 0: 'A'
        uint32_t timeStampsNs; //Offset 1: Nanosecond portion
        uint64_t orderId; // Offset 5: Unique per book/side
        uint32_t orderBookId; // Offset 13: Instrument identifier
        char side; // Offset 17: 'B'=Buy, 'S'=Sell
        uint32_t rankingSeqNum; // Offset 18: Sequential number
        uint64_t quantity; // Offset 22: Visible quantity
        int32_t rawPrice;  // Offset 30: 4-byte Signed Big-Endian
        uint16_t attributes;            // Offset 34 
        uint8_t  lotType;               // Offset 36 
        uint64_t rankingTime;           // Offset 37

    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct orderBookDirectory{
        char messageType;// Offset 0: 'R'
        uint32_t timeStampNs;// Offset 1: Nanoseconds
        uint64_t orderBookID;// Offset 5: Primary identifier
        char symbol[32];// Offset 9: Security short name
        char longName[32];// Offset 41: Human-readable name
        char isin[12];// Offset 73: ISIN code
        uint8_t financialProduct;// Offset 85: Option, Future, Cash, vb.
        char tradingCurrency[3]; // Offset 86: Trading currency
        int16_t numDecimalsPrice;      // Offset 89: Fiyat ondalık sayısı (KRİTİK!) 
        uint16_t numDecimalsNominal;    // Offset 91: Nominal değer ondalığı 
        uint32_t oddLotSize;            // Offset 93: Odd lot miktarı 
        uint32_t roundLotSize;          // Offset 97: Round lot miktarı [cite: 99]
        uint32_t blockLotSize;          // Offset 101: Block lot miktarı [cite: 99]
        uint64_t nominalValue;          // Offset 105: Nominal value [cite: 99]
        uint8_t  numLegs;               // Offset 113: Kombo enstrüman ayak sayısı [cite: 99]
        uint32_t underlyingId;          // Offset 114: Dayanak varlık ID [cite: 99]
        int32_t  strikePrice;           // Offset 118: Price tipinde (Signed) [cite: 99]
        uint32_t expirationDate;        // Offset 122: YYYYMMDD formatında [cite: 99]
        uint16_t numDecimalsStrike;     // Offset 126: Strike price ondalığı [cite: 99]
        uint8_t  putOrCall;             // Offset 128: 1=Call, 2=Put [cite: 99]
        uint8_t  rankingType;           // Offset 129: 1=Price Time [cite: 99]


    };
    #pragma pack(pop)

    #pragma pack(push,1)
    struct time{
        char messageType; //Offset 0: 'T'
        uint32_t seconds; //Unix Time
    };
    #pragma pack(pop)

    #pragma pack(push,1)
    struct combiantionLeg{
        char messageType; //Offset 0: 'M'
        uint32_t timeStampNs; //Offset 1: Nanoseconds
        uint32_t comOrderBookID;// Offset 5
        uint32_t legBookId;         // Offset 9
        char     legSide;           // Offset 13
        uint32_t legRatio;          // Offset 14


    };
    #pragma pack(pop)

    #pragma pack(push,1)
    struct MsgTickSize {
    char     type;              // 'L' 
    uint32_t timestampNs;       // Offset 1 
    uint32_t orderBookId;       // Offset 5 
    uint64_t tickSize;          // Offset 9: 8-byte Numeric 
    int32_t  priceFrom;         // Offset 17: Price
    int32_t  priceTo;           // Offset 21: Price (0=Infinity)
    };
    #pragma pack(pop)

    #pragma pack(push,1)
    struct MsgShortSellStatus {
    char     type;              // 'V' 
    uint32_t timestampNs;       // Offset 1 
    uint32_t orderBookId;       // Offset 5 
    uint8_t  restriction;       // Offset 9: 0=No rest, 1=Not allowed, 2=Up-tick [cite: 135]
    };
    #pragma pack(pop)

    
    #pragma pack(push, 1)
    struct MsgSystemEvent {
    char     type;              // 'S' 
    uint32_t timestampNs;       // Offset 1 
    char     eventCode;         // Offset 5: O=Start, C=End 
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct MsgOrderBookState {
    char     type;              // 'O' 
    uint32_t timestampNs;       // Offset 1 
    uint32_t orderBookId;       // Offset 5 
    char     stateName[20];     // Offset 9: Alpha 
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct MsgOrderExecuted {
    char     type;              // 'E' 
    uint32_t timestampNs;       // Offset 1 
    uint64_t orderId;           // Offset 5 
    uint32_t orderBookId;       // Offset 13 
    char     side;              // Offset 17: B/S 
    uint64_t executedQty;       // Offset 18 
    uint64_t matchId;           // Offset 26 
    uint32_t comboGroupId;      // Offset 34 
    char     reserved[14];      // Offset 38 (7+7 bytes) 
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct MsgOrderExecutedPrice {
    char     type;              // 'C' 
    uint32_t timestampNs;       // Offset 1 
    uint64_t orderId;           // Offset 5 
    uint32_t orderBookId;       // Offset 13 
    char     side;              // Offset 17: B/S 
    uint64_t executedQty;       // Offset 18 
    uint64_t matchId;           // Offset 26 
    uint32_t comboGroupId;      // Offset 34 
    char     reserved[14];      // Offset 38 (7+7 bytes) 
    int32_t  tradePrice;        // Offset 52 
    char     occurredAtCross;   // Offset 56: Y/N 
    char     printable;         // Offset 57: Y/N 
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct MsgOrderDelete {
    char     type;              // 'D' 
    uint32_t timestampNs;       // Offset 1 
    uint64_t orderId;           // Offset 5 
    uint32_t orderBookId;       // Offset 13 
    char     side;              // Offset 17 
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct MsgOrderBookFlush {
    char     type;              // 'Y' 
    uint32_t timestampNs;       // Offset 1 
    uint32_t orderBookId;       // Offset 5 
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct MsgTrade {
    char     type;              // 'p' (Small p!) 
    uint32_t timestampNs;       // Offset 1 
    uint64_t matchId;           // Offset 5 
    uint32_t comboGroupId;      // Offset 13 
    char     side;              // Offset 17 
    uint64_t quantity;          // Offset 18 
    uint32_t orderBookId;       // Offset 26 
    int32_t  tradePrice;        // Offset 30 
    char     reserved[14];      // Offset 34 (7+7 bytes) 
    char     printable;         // Offset 48 
    char     occurredAtCross;   // Offset 49 
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct MsgEquilibriumPrice {
    char     type;              // 'Z' 
    uint32_t timestampNs;       // Offset 1 
    uint32_t orderBookId;       // Offset 5 
    uint64_t bidQtyAtEq;        // Offset 9 
    uint64_t askQtyAtEq;        // Offset 17 
    int32_t  eqPrice;           // Offset 25 
    int32_t  bestBidPrice;      // Offset 29 
    int32_t  bestAskPrice;      // Offset 33 
    uint64_t bestBidQty;        // Offset 37 
    uint64_t bestAskQty;        // Offset 45 
    };
    #pragma pack(pop)

    namespace UTILS{
    inline uint16_t swap16(uint16_t value){return __builtin_bswap16(value);}
    inline uint32_t swap32(uint32_t value){return __builtin_bswap32(value);}
    inline uint64_t swap64(uint64_t val) { return __builtin_bswap64(val); }
    
    //standart price data tipi 4-byte boyutunda 
    static constexpr int32_t NO_PRICE = 0x80000000;//market order için
    inline int32_t decodePrice(int32_t value){
        return static_cast<int32_t>(__builtin_bswap32(static_cast<int32_t>(value)));     
    }

    inline bool isHigherPriority(const InternalOrder& newOrder, const InternalOrder& existingOrder){
    //1.fiyat öncliği (price priority)
    if(newOrder.side == 'B'){//alış emri: fiyat ne kadar yüksekse o kadar öncelikli
        if(newOrder.price > existingOrder.price) return true;
        if (newOrder.price < existingOrder.price) return false;

    }else{//satış emri: fiyat ne kadar düşükse o kadar öncelikli
        if(newOrder .price < existingOrder.price) return true;
        if(newOrder.price > existingOrder.price) return false;

    }
    //2.Zaman önceliği(rankning time priority)
    if (newOrder.rankingTime < existingOrder.rankingTime) return true;
    if (newOrder.rankingTime > existingOrder.rankingTime) return false;
    
    //3. sıra numarası önceliği(ranking sequence priority)
    // Fiyat ve zaman eşitse, sıra numarası daha KÜÇÜK olan önceliklidir
    return (newOrder.rankingSeqNum < existingOrder.rankingSeqNum);

    }

    inline void parseAlpha(char* dest, const char* src, size_t len){
        size_t actualLen = len;
        while(actualLen>0 && (src[actualLen - 1] == ' ' || src[actualLen - 1] == '\0')){
            actualLen--;
        }
        std::memcpy(dest,src, actualLen);
        dest[actualLen] = '\0';
    }
    

    
    }
    
}

