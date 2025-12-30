#include "algorithm.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>

bool Strategy::tokensEqual(const char* lhs, const char* rhs){
    return (std::memcmp(lhs, rhs,14) ==0);
}
Strategy::Strategy(Config cfg, OuchClient client) : globalConfig_(cfg), client_(std::move(client)){
    //config'teki hisseleri state havuzuna taşı
    for(int i=0; i<globalConfig_.instrumentNumber; i++){
        statePool_[i].isActive = true;
        statePool_[i].cfg = globalConfig_.instruments[i];

        std::cout<<"[ALGORITHM] stock init:"<< statePool_[i].cfg.symbol<< "ID: "<<statePool_[i].cfg.targetObid<< "\n";


    }
}
//linear search yapılacak
Strategy::InstrumentState* Strategy::findStateByObid(uint32_t obid){
    for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
        if (statePool_[i].isActive && statePool_[i].cfg.targetObid == obid) {
            return &statePool_[i];
        }
    }
    return nullptr;
}

Strategy::InstrumentState* Strategy::findStateByToken(const char* token){
    for(int i=0; i<MAX_INSTRUMENTS; ++i){
        //aktif değilse geç
        if(!statePool_[i].isActive) continue;
        InstrumentState& inst = statePool_[i];
        if(inst.hasBuyToken && tokensEqual(inst.buyToken, token)) return &inst;
        if(inst.hasSellToken && tokensEqual(inst.sellToken, token)) return &inst;

    }   
    return nullptr;

}


void Strategy::onSnapshot(uint32_t obid, const struct orderBook* book, clock::time_point ts){
    //statik havuzdan bul
    InstrumentState* inst = findStateByObid(obid);
    if(!inst) return;

    processInstrument(*inst, book, ts);

}

//bist tarafından onay gelme mesjaını logluyoruz

void Strategy::onAccepted(const char* token){
    InstrumentState* inst = findStateByToken(token);
    if(inst) std::cout<<"[ALGO] emir bist tarafından onaylandı: "<< inst->cfg.symbol << "\n";

}


//eşleşme/ işlem gerçekleşti
void Strategy::onExecuted(const OUCHExecution& exec){
    //hangi hisseye ait olduğunu bul
    InstrumentState* inst = findStateByToken(exec.token);
    if(!inst) return; //bizim stratejiin emri değil

    //alış eşleşmesi

    if(inst-> hasBuyToken && tokensEqual(inst->buyToken,exec.token)){
        inst->filledBuyQty += exec.quantity;
        //fiyat gğncellemesi
        inst->entryPrice = exec.price;

        //istenen miktar dolud mu?
        if(inst->filledBuyQty >= inst->pendingQty){
            inst->positionQty += inst->filledBuyQty;

            //alış bitti token'i boşa çıkar

            inst->hasBuyToken =false;
            inst->pendingQty = 0;
            inst->filledBuyQty = 0;

            //state: artık long'tayız

            inst->state = OrderState::Long;

            //bekleme sayacını sıfırla
            inst->holdSnapshotCount = 0;

            std::cout << "[ALGO] " << inst->cfg.symbol << " ALIS TAMAMLANDI. Maliyet: " << inst->entryPrice << "\n";
            
        }
    }


    // --- SATIŞ EŞLEŞMESİ ---
    else if (inst->hasSellToken && tokensEqual(inst->sellToken, exec.token)) {
        // Elimizdeki maldan düş
        if (exec.quantity >= inst->positionQty) {
            inst->positionQty = 0;
        } else {
            inst->positionQty -= exec.quantity;
        }

        if (inst->positionQty == 0) {
            // Hepsi satıldı, çıkış tamam
            inst->hasSellToken = false;
            inst->state = OrderState::Idle; // Başa dön
            inst->holdSnapshotCount = 0;
            
            std::cout << "[ALGO] " << inst->cfg.symbol << " POZISYON KAPANDI.\n";
        } else {
            // Kısmi eşleşme (Partial Fill) -> Hala mal var, LONG devam
            // (State PendingSell'de kalabilir veya Long'a çekip tekrar satış denenebilir, 
            // ama basitlik için PendingSell'de bırakıp kalanını satmasını bekliyoruz)
             std::cout << "[ALGO] " << inst->cfg.symbol << " KISMI SATIS. Kalan: " << inst->positionQty << "\n";
        }
    }

}

/// @brief emir iptal oldu
/// @param token 
void Strategy::onCanceled(const char* token){
    InstrumentState* inst = findStateByToken(token);
    if (!inst) return;

    //alış emri iptal olduysa 
    if(inst->hasBuyToken && tokensEqual(inst->buyToken, token)){
        inst->hasBuyToken = false;
        inst->pendingQty = 0;
        inst->filledBuyQty = 0;
        
        // Başa dön ve yeni fırsat bekle
        inst->state = OrderState::Idle;
        inst->holdSnapshotCount = 0;
        
        std::cout << "[ALGO] " << inst->cfg.symbol << " ALIS IPTAL EDILDI.\n";
    }
    //satış emri iptal oluyorsa
    else if(inst->hasSellToken && tokensEqual(inst->sellToken, token)){
        inst->hasSellToken = false;
        // Eğer elimde hala mal varsa LONG durumuna dön (Tekrar satmayı deneriz)
        // Eğer mal yoksa IDLE'a dön
        inst->state = (inst->positionQty > 0) ? OrderState::Long : OrderState::Idle;
        
        std::cout << "[ALGO] " << inst->cfg.symbol << " SATIS IPTAL EDILDI.\n";

    }
}

//token üretici
void Strategy::nextToken(char* buffer){
    //SFT+ 11 haneli sayı üretmek bu bizim tokenimiz olacak
    uint64_t value = tokenCounter++;
    buffer[0] = 'S';
    buffer[1] = 'F';
    buffer[2] = 'T';

    for(int i=13; i>=3; --i){
        //modulo 10 ile son basamağı al '0' ekleyerek ASCII yap
        buffer[i] = (value%10) + '0';

        //sayıyı küçült
        value /=10;
    }

}
void Strategy::processInstrument(InstrumentState& inst, const struct orderBook* book, clock::time_point ts){
    //defter veya fiyatlar boşsa işlem yapma
    if(!book|| !book->bids|| !book->asks) return;

    //en iyi fiyatlar level2 head pointers
    const auto* bestAsk= book->asks;//satıcılar(burdan alım yapılacak)
    const auto* bestBid = book->bids;//alıcılar (buraya satış yapılacak)

    bool forceExit= false;

    //state machine
    switch(inst.state){
        case OrderState::Idle:
            inst.holdSnapshotCount = 0;
            //en iyi satış fiyatlarını geitmeyi dene(best Ask)
            tryEnter(inst, bestAsk->price, bestAsk->totalShares, ts);
            break;
        case OrderState::PendingBuy:
            //token kaybolduys başa dön
            if(!inst.hasBuyToken){
                inst.state = OrderState::Idle;
                std::cout<<"token kayboldu\n";
                break;
            }
            //f,yat kaçtı mı onun kontrolu(Ask retreat)
            // Biz emri 10.00'a attık ama en iyi satış 10.05 oldu. Emrimiz havada kaldı.
            if(globalConfig_.cancelOnAskRetreat && bestAsk ->price> inst.entryPrice){
                abandonBuy(inst);//iptal et
                break;
            }
            //zaman aşımı oldu mu?
            //emri atalı 100 ms geçti ama hala eşleşmedi
            if(globalConfig_.maxPendingBuy.count()>0 && ((ts- inst.buySentTs) > globalConfig_.maxPendingBuy)){
                abandonBuy(inst);
            }
            break;
        // --- DURUM 3: ELİMİZDE MAL VAR (LONG) ---
        case OrderState::Long:
            // Pozisyon tutma süresi (Opsiyonel: çok beklediysek forceExit = true yapılabilir)
            // Basitlik için burada sadece kar/zarar kontrolüne gidiyoruz.
            
            // En iyi alış fiyatına (Best Bid) satmayı dene
            tryExit(inst, bestBid->price, ts, forceExit);
            break;

        // --- DURUM 4: SATIŞ EMRİ GİTTİ ---
        case OrderState::PendingSell:
            // Eğer pozisyon sıfırlandıysa (hepsi satıldıysa) başa dön
            if (inst.positionQty == 0) {
                inst.hasSellToken= false;//hasselltoken
                inst.state = OrderState::Idle;
            }
            break;
    }

}


//giriş kontrolü
void Strategy::tryEnter(InstrumentState& inst, int32_t askPrice, uint32_t askQty, clock::time_point ts) {
    // Sadece IDLE durumunda işlem açarız
    if (inst.state != OrderState::Idle) return;
    
    // OUCH İstemcisi hazır mı?
    if (!client_.sendNewOrder) return;
    
    // Karşıda yeterli likidite (derinlik) var mı?
    // Örn: En az 100 lot satıcı yoksa girme.
    if (askQty < inst.cfg.minAskQty) return;

    // Şartlar uygun, emri ateşle
    sendBuy(inst, askPrice, ts);
}


void Strategy::tryExit(InstrumentState& inst, int32_t bidPrice, clock::time_point ts, bool forceExit) {
    // Sadece LONG durumunda satış yaparız
    if (inst.state != OrderState::Long) return;
    
    // Zaten aktif bir satış emrimiz varsa tekrar gönderme
    if (inst.hasSellToken) return;
    
    // Satacak mal kalmadıysa IDLE'a dön
    if (inst.positionQty == 0) {
        inst.state = OrderState::Idle;
        return;
    }

    // --- KAR AL (TAKE PROFIT) MANTIĞI ---
    if (!forceExit) {
        // Hedef Fiyat = Giriş Fiyatı + Min Kar (Tick)
        // Not: Gerçekte Tick Size (0.01 veya 0.05) ile çarpmak gerekir. 
        // Burada basitçe ham fiyat üzerine ekliyoruz.
        int32_t targetPrice = inst.entryPrice + (int32_t)inst.cfg.minProfitTicks;
        
        // Eğer piyasadaki alıcı (Bid) fiyatı bizim hedefimizin altındaysa SATMA, BEKLE.
        if (bidPrice < targetPrice) {
            return; 
        }
    }

    // Karı gördük veya zorla çıkıyoruz -> SAT
    sendSell(inst, bidPrice, ts);
}


void Strategy::sendBuy(InstrumentState& inst, int32_t price, clock::time_point ts){
    if(inst.hasBuyToken) return;
    char tempToken[14];
    nextToken(tempToken);
    HFT::OUCH::MsgEnterOrder msg;
    msg.set_token(tempToken);
    msg.set_book_id(inst.cfg.targetObid);
    msg.side ='B';
    uint64_t qty = (inst.cfg.orderQty > 0) ? inst.cfg.orderQty : inst.cfg.minAskQty;
    msg.set_qty(qty);
    msg.set_price(price);

    //padding

    msg.set_client(globalConfig_.clientAccount);
    msg.set_exch_info(globalConfig_.exchangeInfo);

    msg.timeInForce = globalConfig_.timeInForce;
    msg.openClose = 'O';
    std::memcpy(inst.buyToken, tempToken,14);
    inst.hasBuyToken = true;
    inst.pendingQty = static_cast<uint32_t>(qty);
    inst.entryPrice = price;
    inst.buySentTs = ts;
    inst.state = OrderState::PendingBuy;

    std::cout<<"[ALGO] BUY("<<inst.cfg.symbol<<"):" <<price<<"\n";

    if(!client_.sendNewOrder(msg)){
        inst.hasBuyToken = false;
        inst.state = OrderState::Idle;
        
    }
}

void Strategy::sendSell(InstrumentState& inst, int32_t price, clock::time_point ts){
    if(!client_.sendNewOrder || inst.positionQty == 0) return;

    char tempToken[14];
    nextToken(tempToken);
    HFT::OUCH::MsgEnterOrder msg;
    msg.set_token(tempToken);
    msg.set_book_id(inst.cfg.targetObid);
    msg.side = 'S';
    msg.set_qty(inst.positionQty);//eldeki tüm malı sat
    msg.set_price(price);

    msg.set_client(globalConfig_.clientAccount);
    msg.set_exch_info(globalConfig_.exchangeInfo);

    msg.timeInForce = globalConfig_.timeInForce;
    msg.openClose = 'C';//close,
    msg.clientCategory = 1;

    //state güncellemesi
    std::memcpy(inst.sellToken, tempToken,14);
    inst.hasSellToken = true;
    inst.state = OrderState::PendingSell;

    std::cout<<"[ALGO] sell(" << inst.cfg.targetObid << ") @ " <<price<<"\n";

    if(!client_.sendNewOrder(msg)){
        inst.hasSellToken = false;
        //satılamazsa long'tan devamedelim
        inst.state = OrderState::Long;
    }
}

void Strategy::abandonBuy(InstrumentState& inst){
    if (!inst.hasBuyToken) return;

    // Eğer Cancel fonksiyonumuz tanımlıysa
    if (client_.sendCancel) {
        HFT::OUCH::MsgCancelOrder cancelMsg;
        // Sadece token yeterli (BIST X mesajı)
        cancelMsg.set_token(inst.buyToken);
        
        client_.sendCancel(cancelMsg);
        // std::cout << "[ALGO] ALIS IPTAL GONDERILDI\n";
    }

    // İptal gönderdikten sonra (veya gönderemesek bile)
    // State'i temizleyip boşa çıkıyoruz.
    // (Gerçek hayatta iptal onayını ('C' mesajı) beklemek daha güvenlidir
    // ama FirstTouch stratejilerinde hız için genelde optimist reset yapılır).
    inst.hasBuyToken = false;
    inst.pendingQty = 0;
    inst.filledBuyQty = 0;
    inst.state = OrderState::Idle;
}


