#ifndef BACKEND_LINUX_UART_H
#define BACKEND_LINUX_UART_H

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <vector>
#include <queue>

using std::vector;
using std::queue;

#include "backend.h"
#include <knx/ringbuffer.h>

#define UART_RINGBUFSIZE 256

namespace KNX {

class LinuxUART : public Backend {
public:
    LinuxUART(const char *device) : Backend(), _pktbuffer(UART_RINGBUFSIZE),
                                    _device(device), _fd(0), _remaining(0) { }

    bool init() {
        if(_ready)
            return false;

        struct termios c;
        _fd = open(_device, O_RDWR);

        if(_fd == -1)
            return false;

        tcgetattr(_fd, &c);
        if(cfsetspeed(&c, B19200) < 0) {
            close(_fd);
            return false;
        }

        cfmakeraw(&c);
        tcsetattr(_fd, TCSANOW, &c);

        _ready = true;
        return true;
    }

    bool broadcast(const telegram &data) {
        if(!_ready)
            return false;

        return write(_fd, data.raw(), data.size());
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
        if(_ready) 
            _ready = false;
        if(_fd) {
            close(_fd); 
            _fd = 0;
        }
    }

    bool must_update() const { 
        return false;
    }

    bool update() {
        if(!_ready) return false;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(_fd, &rfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;

        NETBENCHMARK_START(uart_waiting);
        int n = select(_fd+1, &rfds, NULL, NULL, &tv);
        if(n < 0) {
            NETBENCHMARK_STOP(uart_waiting);
            LOG("Select returned -1.\n");
            return false;
        }
        NETBENCHMARK_STOP(uart_waiting);

        NETBENCHMARK_START(uart_read);
        if(n > 0) {
            uint8_t buf[128];
            int m = ::read(_fd, (char *)buf, 128);
            if(m < 0) {
                NETBENCHMARK_STOP(uart_read);
                LOG("Recv returned -1.");
                return false;
            }

            _process_packet(buf, (size_t)m);
        }
        NETBENCHMARK_STOP(uart_read);

        return true;
    }
private:

    /** Data Stream -> Array of KNX Telegrams */
    void _process_packet(const uint8_t *buf, size_t len) {
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
    const char *_device;
    int _fd;
    size_t _remaining;

    TAG_DEF("Linux-UART")
};

} /* KNX */
#endif /* BACKEND_LINUX_UART_H */
