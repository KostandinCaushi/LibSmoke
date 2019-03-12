#ifndef KNX_TELEGRAM_HH
#define KNX_TELEGRAM_HH 

#include "common.h"
#include "config.h"

namespace KNX {

enum tg_size {
    max = 22,
    hdr = 6,
    payload = 15,
    /* max == hdr + payload + 1 (checksum) */
};

class telegram
{
public:
    enum level
    {
        system = 0,
        high   = 1,
        alarm  = 2,
        normal = 3,
    };

    enum dest_type
    {
        individual = 0,
        group = 1,
    };

    enum address
    {
        broadcast = 0x00,
    };

    struct config
    {
        uint16_t dest;
        level priority;
        dest_type dtype;
        uint8_t routing_cnt;
    };

    explicit telegram(const config &config)
    {
        pkt.hdr.ctrl  = 0xB0 | (config.priority << 2); 
        pkt.hdr.src = toBigEndian16(sKConfig.id()); 
        pkt.hdr.dest = toBigEndian16(config.dest); 
        pkt.hdr.group = config.dtype;
        pkt.hdr.routing = config.routing_cnt;
    } 

    explicit telegram() {
        pkt.hdr.src = toBigEndian16(sKConfig.id());
    }

    ~telegram() { }

    bool write(const uint8_t *data, uint8_t size)
    {
        if(size > tg_size::payload)
            return false;

        memcpy(pkt.body, data, size);
        pkt.hdr.size = size;
        pkt.body[size] = checksum();
        return true;
    }

    /** Getters */
    const uint8_t *raw() const { return pkt.raw; }
    uint8_t *body() { return pkt.body; }
    uint8_t ctrl() const { return pkt.hdr.ctrl; }
    uint16_t src() const { return fromBigEndian16(pkt.hdr.src); }
    uint16_t dest() const { return fromBigEndian16(pkt.hdr.dest); }
    uint8_t size() const { return tg_size::hdr + pkt.hdr.size + 1; }
    uint8_t bodysize() const { return pkt.hdr.size; }

    bool check() const 
    { 
        bool r = ((pkt.hdr.ctrl & 0b11010011) == 0b10010000) && 
                 (checksum() == pkt.body[pkt.hdr.size]); 

        if(r == false)
            LOG("Nothing.. Just checked an invalid telegram...");

        return r;
    }

    level priority() const 
    { 
        return static_cast<level>((pkt.hdr.ctrl >> 2) & 0x03); 
    }

    bool read(uint8_t *data, uint8_t size)
    {
        if(size < pkt.hdr.size)
            return false;

        memcpy(data, pkt.body, pkt.hdr.size);
        return true;
    }

    /** Used only with TP-UART protocol. Read data doesn't have a checksum */
    uint8_t checksum() const
    { 
        uint8_t sum = 0;
        for (uint8_t i = 0; i < size() - 1; ++i)
            sum ^= pkt.raw[i];
        return ~sum;
    }

    uint8_t operator[](uint8_t i) const { return pkt.raw[i]; }
    uint8_t &operator[](uint8_t i) { return pkt.raw[i]; }
private:

    /** KNX Packet */
    #pragma pack(1)
    union
    {
        struct
        {
            struct
            {
                uint8_t ctrl    :  8;
                uint16_t src    : 16;
                uint16_t dest   : 16;
                uint8_t size    :  4;
                uint8_t routing :  3;
                uint8_t group   :  1;
            } hdr;
            uint8_t body[tg_size::payload+1];
        };
        uint8_t raw[tg_size::max];
    } pkt;
    #pragma pack()

    TAG_DEF("Telegram")
}; /* class telegram */

}; /* KNX */

#endif /* ifndef KNX_TELEGRAM_HH */
