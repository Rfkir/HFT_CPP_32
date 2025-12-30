#ifndef HFT_ALGORITHM_HPP
#define HFT_ALGORITHM_HPP

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <cstring>
#include "HFT_OUCH.hpp"
#include "orderBook.hpp"

#define MAX_INSTRUMENTS 5 // Maksimum enstrüman sayısı

// OUCH istemcisi arayüzü
struct OuchClient{
    using sendOrderFn = std::function<bool(const HFT::OUCH::MsgEnterOrder&)>;
    using cancelOrderFn = std::function<void (const HFT::OUCH::MsgCancelOrder&)>;
    
    sendOrderFn sendNewOrder; // Yazım düzeltildi: sendNeworder -> sendNewOrder
    cancelOrderFn sendCancel;
};

// Algoritmaya gelen execution bilgisi
struct OUCHExecution{
    char token[14];
    char side{'B'};
    uint32_t quantity{0};
    int32_t price{0};
};

class Strategy{
    public:
        using clock = std::chrono::steady_clock;

        // 1. Motor Ayar Bilgileri (Tekil Hisse İçin)
        struct InstrumentConfig{
            uint32_t targetObid{0}; // Hedef order book ID
            char symbol[16]{};
            uint32_t minAskQty{1};
            uint32_t orderQty{10};      // İşlem adedi
            uint32_t minProfitTicks{1}; // DÜZELTME: bool -> uint32_t (Tick sayısı)
        };

        // 2. Global Config
        struct Config{
            bool cancelOnAskRetreat{true};
            clock::duration maxPendingBuy{std::chrono::milliseconds(100)}; // 100 ms bekleme
            bool simulateOnly{false}; // DÜZELTME: simulateOnlly -> simulateOnly

            // Hesap bilgileri
            char clientAccount[17] {"HFT_MAIN"};
            char exchangeInfo[33] {"ACC_1234"};
            char timeInForce {'0'};
            char capacity{'A'};

            // Statik config listesi
            int instrumentNumber = 0;
            InstrumentConfig instruments[MAX_INSTRUMENTS];

            // Config ekleme yardımcıları
            void addInstrument(uint32_t id, const char* sym){
                if(instrumentNumber >= MAX_INSTRUMENTS) return;
                instruments[instrumentNumber].targetObid = id;
                std::strncpy(instruments[instrumentNumber].symbol, sym, 15);
                instrumentNumber++;
            }
        };

        Strategy(Config cfg, OuchClient client);

        void onSnapshot(uint32_t obid, const struct orderBook* book, clock::time_point ts);
        void onAccepted(const char* token);
        void onExecuted(const OUCHExecution& exec);
        void onCanceled(const char* token);

    private:
        enum class OrderState{Idle, PendingBuy, Long, PendingSell};

        // 3. Her hissenin anlık durumu
        struct InstrumentState{
            bool isActive{false};
            
            // DÜZELTME: Config -> InstrumentConfig (Sadece bu hissenin ayarı)
            InstrumentConfig cfg; 
            
            OrderState state{OrderState::Idle};

            // Tokenler
            bool hasBuyToken{false};  // DÜZELTME: hash -> has
            char buyToken[14];
            
            bool hasSellToken{false}; // DÜZELTME: hash -> has
            char sellToken[14];

            // Pozisyon
            uint32_t pendingQty{0};
            uint32_t filledBuyQty{0};
            uint32_t positionQty{0};
            int32_t entryPrice{0};
            clock::time_point buySentTs{};
            std::size_t holdSnapshotCount{0};

            InstrumentState(){
                std::memset(buyToken, ' ', 14);
                std::memset(sellToken, ' ', 14);
            }
        };

    // Helpers
    void nextToken(char* buffer);
    bool tokensEqual(const char* lhs, const char* rhs);

    // İş mantığı
    void processInstrument(InstrumentState& inst, const struct orderBook* book, clock::time_point ts);
    void tryEnter(InstrumentState& inst, int32_t askPrice, uint32_t askQty, clock::time_point ts);
    void tryExit(InstrumentState& inst, int32_t bidPrice, clock::time_point ts, bool forceExit);

    // Yazım hataları düzeltildi
    void sendBuy(InstrumentState& inst, int32_t price, clock::time_point ts);
    void sendSell(InstrumentState& inst, int32_t price, clock::time_point ts);
    void abandonBuy(InstrumentState& inst); // abondon -> abandon

    // Hızlı erişim yardımcılar
    InstrumentState* findStateByObid(uint32_t obid);
    InstrumentState* findStateByToken(const char* token);

    Config globalConfig_;
    OuchClient client_;
    uint64_t tokenCounter{0};

    // Statik state havuzu 
    InstrumentState statePool_[MAX_INSTRUMENTS];
};

#endif