#ifndef BACKEND_LINUX_TPUART_H
#define BACKEND_LINUX_TPUART_H

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

class LinuxTPUART : public Backend {
public:
    LinuxTPUART(const char *device) : Backend(), _pktbuffer(UART_RINGBUFSIZE),
                                      _device(device), _fd(0),
                                      _is_packet_outgoing(false){ }

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
        c.c_cflag |= PARENB;

        tcsetattr(_fd, TCSANOW, &c);

        if(!_reset()) {
            LOG("reset() failed.");
            return false;
        }

        _set_address(sKConfig.id()); 

        if(!_check_status()) {
            LOG("check_status() failed");
            close(_fd);
            return false;
        }

        _ready = true;
        return true;
    }

    bool broadcast(const telegram &data) {
        static const size_t output_max_size = 16;
        if(_output.size() > output_max_size) {
            LOG("Output queue full. Discarding packet!");
            return false;
        }

        _output.push(data);
        return true;
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
        return _output.size() > 0; 
    }

    bool update() {
        if(!_ready) return false;

        NETBENCHMARK_START(uart_writing);
        _process_outgoing();
        NETBENCHMARK_STOP(uart_writing);

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
            LOG("Select returned -1.");
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

    void _process_outgoing() {
        if(!_is_packet_outgoing && _output.size() > 0) {
            _the_outgoing = _output.front();
            _is_packet_outgoing = true;
            _output.pop();
            _send_telegram();
        }
    }

    bool _send_telegram() {
        const telegram& data = _the_outgoing;
        ssize_t n = 0;
        char buf;

        if(!_ready)
            return false;

        if(data.size() == 0)
            return true;

        for(size_t i = 0; i < (size_t)(data.size() - 1); i++) {
            buf = 0x80 | i;
            n += write(_fd, &buf, 1);
            n += write(_fd, &data.raw()[i], 1);
        }

        buf = 0x40 | (data.size() - 1);
        n += write(_fd, &buf, 1);
        n += write(_fd, &data.raw()[data.size()-1], 1);

        return (n == 2 * data.size());
    }

    /** Data Stream -> Array of KNX Telegrams */
    void _process_packet(const uint8_t *buf, size_t len) {
        if(len == 0) return; 

        _pktbuffer.write(buf, len);

        while(_pktbuffer.size() > 0) {
            uint8_t hdr[tg_size::hdr];
            /* At first, read only the opcode */
            _pktbuffer.peek((uint8_t *)&hdr, 1);

            if((hdr[0] & 0x7f) == 0x0b) {
                /* Confirm opcode? */
                bool ok = hdr[0] >> 7;
                if(_is_packet_outgoing && !ok) {
                    LOG("Failed to send. Discarding.");
                    _is_packet_outgoing = false;
                } else {
                    if(!_is_packet_outgoing) 
                        LOG("L_DATA.confirm without outgoing packet");
                    _is_packet_outgoing = false;
                }

                _pktbuffer.remove(1);
                continue;
            } else if((hdr[0] & 0x07) == 0x07) {
                /* Some kind of error code? */
                LOGP("Error code: %02x", hdr[0]);
                _pktbuffer.remove(1);
                continue;
            } else if((hdr[0] & 0b11010011) == 0b10010000) {
                /* Data packet? */
                if(_pktbuffer.size() < tg_size::hdr)
                    break;

                _pktbuffer.peek((uint8_t *)&hdr, sizeof(hdr));
                /* dsize can be maximum tg_size::hdr + 15 = 21 (< 22). :) */
                uint8_t dsize = tg_size::hdr + 1 + (hdr[5] & 0x0f); 

                if(dsize > _pktbuffer.size())
                    return;

                telegram pkt;
                _pktbuffer.read(&pkt[0], dsize);
                if(pkt.src() != sKConfig.id() && pkt.check())
                        _pkts.push(pkt);
            } 
        }
    }

    bool _reset() {
        const int n_tries = 5;

        for(int i=0; i < n_tries; i++) {
            char buf = 0x01;
            write(_fd, &buf, 1);

            /* Try to read the status NN times. */
            size_t nn_tries = 5;
            do {
                usleep(100000);
                ssize_t n = ::read(_fd, &buf, 1);
                if(n == 1 && buf == 0x03)
                    return true;
            } while(--nn_tries > 0);
        }

        return false;
    }

    void _set_address(uint16_t addr) {
        uint8_t buf[3];
        uint16_t *apos = reinterpret_cast<uint16_t *>(&buf[1]);

        buf[0] = 0x28;
        *apos = toBigEndian16(addr);

        Debug::printArray(buf, 3);
        write(_fd, buf, 3);
    }

    /* TODO: what happens if the KNX device receives some data before 
     * the *only* _get_status() is called?
     */
    bool _check_status() const {
        char buf = 0x02;
        write(_fd, &buf, 1);

        do {
            ssize_t n = ::read(_fd, &buf, 1);
            if(n != 1) 
                return false;

        /* Maybe there are pending 'Reset OK' */
        } while(buf == 0x03 || buf == 0x00); 

        LOGP("check_status: status %02x", buf);
        return (buf == 0x07);
    }

    queue<telegram> _output;
    queue<telegram> _pkts;
    RingBuffer _pktbuffer;
    const char *_device;
    int _fd;
    bool _is_packet_outgoing; 
    telegram _the_outgoing;

    TAG_DEF("LinuxTPUART")
};

} /* KNX */
#endif /* BACKEND_LINUX_TPUART_H */
