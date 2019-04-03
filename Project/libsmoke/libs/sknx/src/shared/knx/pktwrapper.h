#ifndef KNX_PACKET_WRAPPER_H
#define KNX_PACKET_WRAPPER_H

#include <knx/common.h>
#include <knx/telegram.h>
#include <knx/ringbuffer.h>
#include <knx/multirings.h>

namespace KNX
{

/* Opcodes are wrapped in a uint8_t container. Hence, the range is [0x00,0xff]
 * Opcodes in range [0x10,0x1f] are reserved for node counting.
 * Opcodes in range [0x20,0x2f] are reserved for key exchanging.
 */
enum Opcodes {
    CMD_INVALID = 0xff,
    CMD_PING    = 0x00,
};

struct pkt_t {
    uint8_t cmd;
    uint16_t src;
    uint16_t dest;
    std::vector<uint8_t> data;

    // method to get data
    std::vector<uint8_t> getData () {
        return data;
    }
};

class PKTWrapper {
public:
    PKTWrapper(size_t max_clients, Backend &b) : _backend(b),
                        _buffers(max_clients) { }

    inline bool write(uint16_t dest, uint8_t cmd) {
        return write(dest, cmd, NULL, 0);
    }

    inline bool write(uint8_t cmd) {
        return write(cmd, NULL, 0);
    }

    inline bool write(uint8_t cmd, const uint8_t *data, uint16_t len) {
        return write(telegram::address::broadcast, cmd, data, len);
    }

    bool write(uint16_t dest, uint8_t cmd, const uint8_t *buf, 
                uint16_t len) {
        size_t totalsize = len + sizeof(cmd) + sizeof(len);
        uint8_t raw[totalsize];
        uint16_t *size = reinterpret_cast<uint16_t *>(&raw[1]);

        raw[0] = cmd;
        *size = toBigEndian16(len);

        memcpy(&raw[3], buf, len);
        _packetize(dest, raw, totalsize);
        return true;
    }

    bool read(pkt_t &pkt) {
        if(_in.empty()) 
            return false;

        pkt = _in.front();
        _in.pop();
        return true;
    }

    size_t count() const { 
        return _in.size(); 
    }

    void update() {
        _backend.update();

        // Send all telegrams
        while(!_out.empty()) {
            _backend.broadcast(_out.front());
            _out.pop();
        }
        
        // Recv new telegrams
        while(_backend.count() > 0) {
            telegram pkt;
            _backend.read(pkt);

            if(pkt.dest() == telegram::address::broadcast ||
                pkt.dest() == sKConfig.id())
            _recompose(pkt);
        }
    }

private:
    Backend& _backend;
    MultiRings<uint16_t, RingBuffer> _buffers;
    std::queue<telegram> _out;
    std::queue<pkt_t> _in;

    void _recompose(telegram& pkt) {
        // Skip my own telegrams
        if(pkt.src() == sKConfig.id())
            return;

        if(!pkt.check()) {
            LOG("Invalid checksum. Discarding packet.\n");
            return;
        }

        RingBuffer *tempPkt = _buffers.get(pkt.src());
        if(!tempPkt) {
            LOG("RingBuffer just gone out of memory.\n");
            return;
        }

        NETBENCHMARK_START(pktwrapper_recompose);
        tempPkt->write(pkt.body(), pkt.bodysize());

        while(tempPkt->size() >= 3) {
            uint8_t hdr[3];
            pkt_t newPkt;
            tempPkt->peek(hdr, 3);
            uint16_t *szptr = reinterpret_cast<uint16_t *>(&hdr[1]);
            uint16_t dsize = fromBigEndian16(*szptr);
            
            if(tempPkt->size() < (size_t)(dsize + 3))
                break;

            // New "complete" packet
            tempPkt->remove(3);

            newPkt.src = pkt.src();
            newPkt.dest = pkt.dest();
            newPkt.cmd = hdr[0];
            newPkt.data.resize(dsize);

            tempPkt->read(&newPkt.data[0], newPkt.data.size());
            _in.push(newPkt); 
        }
        NETBENCHMARK_STOP(pktwrapper_recompose);
    }
    
    void _packetize(uint16_t dest, const uint8_t *buf, size_t len) {
        using std::min;
        const size_t count = len / tg_size::payload + 
                                ((len % tg_size::payload) > 0 ? 1 : 0);

        NETBENCHMARK_START(pktwrapper_packetize);
        for(size_t i=0; i<count; i++) {
            telegram::config tgconfig = { dest, telegram::level::normal,
                                          telegram::dest_type::group, 0}; 
            
            if (dest != telegram::address::broadcast)
                tgconfig.dtype = telegram::dest_type::individual;

            telegram pkt(tgconfig);
            size_t sz = tg_size::payload;
            if(len - (sz * i) < sz)
                sz = len - (sz * i);

            pkt.write(buf + i * tg_size::payload, sz);
            _out.push(pkt);
        }
        NETBENCHMARK_STOP(pktwrapper_packetize);
    }
    
    TAG_DEF("PKTWrapper") 
};

} /* KNX */

#endif /* ifndef KNX_PACKET_WRAPPER_H */
