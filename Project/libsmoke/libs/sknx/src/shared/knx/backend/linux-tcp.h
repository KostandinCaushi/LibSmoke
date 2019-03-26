#ifndef BACKEND_TCPSOCKET_HH
#define BACKEND_TCPSOCKET_HH

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <queue>

using std::vector;
using std::queue;

#include "backend.h"
#include <knx/ringbuffer.h>

#define TCP_RINGBUFSIZE 256

namespace KNX {

template<uint16_t PORT>
class LinuxTCP : public Backend {
public:
    LinuxTCP(const char &addr, unsigned short &port) : Backend(), _pktbuffer(TCP_RINGBUFSIZE), _socket(0),
                 _remaining(0), in.sin_family(AF_INET), in.sin_port(htons(port)),
                in.sin_addr.s_addr(inet_addr(addr)) { }

    bool init() {
        if(_ready)
            return false;

        if((_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
            return false;

        if(connect(_socket, (struct sockaddr *)&in, sizeof(in)) != 0) {
            close(_socket);
            return false;
        }

        _ready = true;
        return true;
    }

    bool broadcast(const telegram &data) {
        if(!_ready)
            return false;

        return send(_socket, data.raw(), data.size(), 0);
    }

    bool read(telegram &data) {
        if(!_ready || _pkts.size() == 0)
            return false;
        data = _pkts.front();
        
        _pkts.pop();
        return true;
    }

    void pop() {
        if(_pkts.size() > 0)
            _pkts.pop();
    }

    size_t count() const {
        return _pkts.size();
    }

    void shutdown() {
        if(_ready) _ready = false;
        if(_socket) close(_socket);
    }

    bool must_update() const { 
        return false; 
    }

    bool update() {
        if(!_ready) return false;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(_socket, &rfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;

        NETBENCHMARK_START(tcp_waiting);
        int n = select(_socket+1, &rfds, NULL, NULL, &tv);
        if(n < 0) {
            NETBENCHMARK_STOP(tcp_waiting);
            LOG("Select returned -1.\n");
            return false;
        }
        NETBENCHMARK_STOP(tcp_waiting);

        NETBENCHMARK_START(tcp_read);
        if(n > 0) {
            uint8_t buf[128];
            int m = recv(_socket, (char *)buf, 128, 0);
            if(m < 0) {
                NETBENCHMARK_STOP(tcp_read);
                LOG("Recv returned -1.");
                return false;
            }

            _process_packet(buf, (size_t)m);
        }
        NETBENCHMARK_STOP(tcp_read);

        return true;
    }
private:

    /** TCP Stream -> Array of KNX Telegrams */
    void _process_packet(uint8_t *buf, size_t len) {
        if(len == 0) return; 

        _pktbuffer.write(buf, len);

        while(_pktbuffer.size() >= tg_size::hdr + 1) {
            uint8_t hdr[tg_size::hdr];
            _pktbuffer.peek((uint8_t *)&hdr, sizeof(hdr));

            uint8_t dsize = tg_size::hdr + 1 + (hdr[5] & 0x0f);

            if(dsize > _pktbuffer.size())
                return;

            telegram pkt;

            _pktbuffer.read(&pkt[0], dsize);
            _pkts.push(pkt);
        }
    }

    queue<telegram> _pkts;
    RingBuffer _pktbuffer;
    int _socket;
    size_t _remaining;
    struct sockaddr_in in;

    TAG_DEF("LinuxTCP")
};

} // namespace KNX
#endif /* ifndef BACKEND_TCPSOCKET_HH */
