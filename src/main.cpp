#include "algorithm.hpp"
#include "HFT_ITCH.hpp"
#include "HFT_OUCH.hpp"
#include "orderBook.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <string>

// Global Değişkenler
struct orderBook myBook;
std::ofstream ouchFile;

// ============================================================================
// 1. HEX STRING -> BYTE DÖNÜŞTÜRÜCÜ
// ============================================================================
// 'A' -> 10, 'F' -> 15, '0' -> 0 çevirisi yapar
uint8_t hexCharToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

// ============================================================================
// 2. LOGLAMA (Aynı)
// ============================================================================
void write_hex_dump(const char* label, const void* data, size_t len) {
    if (!ouchFile.is_open()) return;
    const uint8_t* byteData = (const uint8_t*)data;
    ouchFile << label << " ";
    ouchFile << std::hex << std::uppercase << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ouchFile << std::setw(2) << (int)byteData[i];
    }
    ouchFile << "\n";
    ouchFile.flush();
    ouchFile << std::dec << std::nouppercase;
}

// ============================================================================
// 3. MOCK CLIENT
// ============================================================================
bool mock_send_order(const HFT::OUCH::MsgEnterOrder& msg) {
    write_hex_dump("O", &msg, sizeof(msg));
    return true; 
}
void mock_cancel_order(const HFT::OUCH::MsgCancelOrder& msg) {
    write_hex_dump("X", &msg, sizeof(msg));
}

// ============================================================================
// 4. MAIN (HEX OKUYUCU)
// ============================================================================
int main(int argc, char* argv[]) {
    // 1. Çıktı Dosyası
    ouchFile.open("ouch_orders.txt", std::ios::out | std::ios::trunc);
    if (!ouchFile.is_open()) return -1;

    // 2. Dosya Adı (Parametre veya Sabit)
    std::string filename = "itch_hex_dump.txt"; // Varsayılan ad
    if (argc > 1) filename = argv[1];

    std::ifstream file(filename); // Text modunda açıyoruz
    if (!file.is_open()) {
        std::cerr << "[HATA] Dosya acilamadi: " << filename << "\n";
        return -1;
    }
    std::cout << "[INFO] Hex verisi okunuyor: " << filename << "\n";

    // 3. Strateji Kurulumu
    Strategy::Config cfg;
    cfg.addInstrument(10, "ISCTR.E"); 
    cfg.simulateOnly = true;
    OuchClient client;
    client.sendNewOrder = mock_send_order;
    client.sendCancel = mock_cancel_order;
    Strategy strategy(cfg, client);

    // 4. OKUMA DÖNGÜSÜ
    // Hex veriyi Byte'a çevirip bir Buffer'a dolduracağız
    // ITCH mesajları değişken uzunlukta olduğu için bir "Stream Parser" mantığı gerekir.
    // Ancak BIST ITCH mesajlarının ilk byte'ı "Message Type"tır. Uzunluk oradan bellidir.
    // Veya dosyan "Length + Message" formatındaysa ona göre okumalıyız.
    
    // Varsayım: Dosya sürekli bir Hex akışı (örn: "000C41..." -> 2 byte len + msg)
    
    std::vector<uint8_t> packetBuffer;
    char c;
    char hexPair[2];
    int pairIndex = 0;
    long long msgCount = 0;

    // Karakter karakter oku
    while (file.get(c)) {
        // Boşluk, yeni satır vb. atla
        if (isspace(c)) continue; 

        // Geçerli Hex karakteri mi?
        if (!isxdigit(c)) continue;

        hexPair[pairIndex++] = c;

        // İkili tamamlandıysa (Örn: "4" ve "A" okundu -> 0x4A)
        if (pairIndex == 2) {
            uint8_t byteVal = (hexCharToInt(hexPair[0]) << 4) | hexCharToInt(hexPair[1]);
            packetBuffer.push_back(byteVal);
            pairIndex = 0;

            // --- PAKET AYRIŞTIRMA (PARSING) ---
            // Burası dosyanın yapısına göre değişir.
            // Eğer dosyan "2 Byte Length + Message" yapısındaysa:
            
            if (packetBuffer.size() >= 2) {
                // İlk 2 byte (Uzunluk) okundu mu?
                uint16_t msgLen = (packetBuffer[0] << 8) | packetBuffer[1]; // Big Endian varsayımı
                
                // Tam paket elimize ulaştı mı? (2 byte header + msgLen kadar veri)
                if (packetBuffer.size() == (size_t)(2 + msgLen)) {
                    
                    // Veriyi işle (Header olan ilk 2 byte hariç)
                    process_buffer(packetBuffer.data() + 2);

                    // Stratejiyi tetikle
                    auto now = std::chrono::steady_clock::now();
                    strategy.onSnapshot(10, &myBook, now);

                    msgCount++;
                    if (msgCount % 10000 == 0) std::cout << "." << std::flush;

                    // Buffer'ı temizle, sıradaki paket için hazırlan
                    packetBuffer.clear();
                }
            }
        }
    }

    std::cout << "\n[INFO] Islem bitti. Toplam Mesaj: " << msgCount << "\n";
    file.close();
    ouchFile.close();
    return 0;
}