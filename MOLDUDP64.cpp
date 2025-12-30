#include <iostream>
#include <cstdint>
//#include <arpa/inet.h> // ntohs, be64toh vb. için (Linux)

// MoldUDP64 Header Yapısı (Standart)
#pragma pack(push, 1)
struct MoldHeader {
    char     Session[10];  // Oturum ismi (ASCII)
    uint64_t Sequence;     // Paketin ilk mesajının sıra numarası
    uint16_t Count;        // Paketin içindeki mesaj sayısı
};
#pragma pack(pop)

// process_buffer fonksiyonunu artık tek mesaj için değil,
// bütün bir UDP paketini işlemek için güncelliyoruz.
void process_udp_packet(const uint8_t* buffer, size_t len) {
    if (len < 20) return; // Header bile sığmıyorsa çöptür

    // 1. Header'ı Oku
    const MoldHeader* header = (const MoldHeader*)buffer;
    
    // Mesaj sayısını al (Big Endian -> Host Endian)
    uint16_t msgCount = ntohs(header->Count);
    
    // Pointer'ı Header'ın (20 byte) sonuna kaydır
    // Artık payload kısmındayız
    const uint8_t* ptr = buffer + 20;
    
    // Güvenlik için kalan uzunluk hesabı
    size_t remainingLen = len - 20;

    // 2. Mesajları Döngüyle İşle
    for (int i = 0; i < msgCount; ++i) {
        if (remainingLen < 2) break; // Uzunluk verisi bile kalmadıysa çık

        // MoldUDP64 payload'ında her mesajın başında 2 byte UZUNLUK (Length) bilgisi olur
        uint16_t msgLen = ntohs(*(const uint16_t*)ptr);
        
        ptr += 2; // Uzunluk bilgisini geç, mesajın kendisine gel
        remainingLen -= 2;

        if (remainingLen < msgLen) {
            // Hata: Paket yarım kalmış veya bozuk
            std::cout << "[ERR] Eksik paket verisi!\n";
            break; 
        }

        // --- BURASI SENİN HANDLER'I ÇAĞIRDIĞIN YER ---
        // ptr şu an 'A', 'E', 'D' vb. mesaj tipinin olduğu byte'ı gösteriyor.
        process_buffer(ptr); 
        // ---------------------------------------------

        // Bir sonraki mesaja zıpla
        ptr += msgLen;
        remainingLen -= msgLen;
    }
}