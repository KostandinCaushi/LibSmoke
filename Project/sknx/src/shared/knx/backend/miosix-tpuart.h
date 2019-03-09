#ifndef BACKEND_MIOSIX_TPUART_H
#define BACKEND_MIOSIX_TPUART_H

#ifndef _MIOSIX
#error "This backend is available only using Miosix."
#endif

#include <miosix.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/sync.h>
#include <drivers/serial_stm32.h>
#include <queue>
#include <deque>
#include <memory>

#include "backend.h"
#include <knx/ringbuffer.h>

#define UART_RINGBUFSIZE 256

using miosix::Lock;
using miosix::FastMutex;
using miosix::Thread;
using miosix::STM32Serial;

void _miosix_tpuart_recv_thread(void *arg);

namespace KNX {

class MiosixTPUART : public Backend {
public:
    MiosixTPUART(int device, int baudrate) : 
        Backend(), _serial(device, baudrate), _pktbuffer(UART_RINGBUFSIZE), 
        _device(device - 1), _is_packet_outgoing(false) {
            switch(device) {
                case 1: _port = USART1; break;
                case 2: _port = USART2; break;
#if !defined(STM32F411xE) && !defined(STM32F401xC)
                case 3: _port = USART3; break;
#else
                case 3: _port = USART6; break;
#endif
                default: _port = NULL; break;
            }
        }

    ~MiosixTPUART() {
        shutdown();
    }

    bool init() {
        Lock<FastMutex> l(_usart_mutex);

        if(_device < 0 || _device >= (ssize_t) sizeof(_miosix_usart_locked)) {
            LOGP("Cannot initialize an invalid USART device %d", _device);
            return false;
        }

        if(_ready || _miosix_usart_locked[_device] || _port == NULL)
            return false;

        _miosix_usart_locked[_device] = true;

        _port->CR1 |= USART_CR1_PCE | USART_CR1_M;

        if(!_reset()) {
            LOG("reset() failed.");
            _port->CR1 &= ~(USART_CR1_PCE | USART_CR1_M);
            return false;
        }

        _set_address(sKConfig.id()); 

        if(!_check_status()) {
            LOG("check_status() failed");
            _port->CR1 &= ~(USART_CR1_PCE | USART_CR1_M);
            return false;
        }

        Thread::create(_miosix_tpuart_recv_thread, 1024, 
                miosix::MAIN_PRIORITY, reinterpret_cast<void *>(this));
        _ready = true;

        return true;
    }

    bool broadcast(const telegram &data) {
        static const size_t output_max_size = 16;

        if(!_ready)
            return false;

        while(_output.size() > output_max_size) {
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
        Lock<FastMutex> l(_usart_mutex);

        if(_ready) 
            _ready = false;

        if(_device < 0 || _device >= (ssize_t) sizeof(_miosix_usart_locked))
            return;

        _miosix_usart_locked[_device] = false;
    }

    bool must_update() const { 
        return _output.size() > 0; 
    }

    bool update() {
        Lock<FastMutex> l(_mutex);

        if(!_ready) 
            return false;

        NETBENCHMARK_START(uart_writing);
        _process_outgoing();
        NETBENCHMARK_STOP(uart_writing);

        if(_stream.size() == 0)
            return true;

        NETBENCHMARK_START(uart_read);
        _process_packet(&_stream[0], _stream.size());
        _stream.clear();
        NETBENCHMARK_STOP(uart_read);

        return true;
    }

    /* Don't call manually functions below */
    void _thread_push_data(const uint8_t *data, size_t len) {
        Lock<FastMutex> l(_mutex);
        for(size_t i=0; i<len;i++)
            _stream.push_back(data[i]);
    }

    STM32Serial& _thread_get_device() {
        return _serial;
    }

    const int _thread_get_device_id() const { return _device; }
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
            n += _serial.writeBlock(&buf, 1, 0);
            n += _serial.writeBlock(&data.raw()[i], 1, 0);
        }

        buf = 0x40 | (data.size() - 1);
        n += _serial.writeBlock(&buf, 1, 0);
        n += _serial.writeBlock(&data.raw()[data.size()-1], 1, 0);

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
            _serial.writeBlock(&buf, 1, 0);

            /* Try to read the status NN times. */
            size_t nn_tries = 5;
            do {
                Thread::sleep(100);
                ssize_t n = _serial.readBlock(&buf, 1, 0);
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

        _serial.writeBlock(buf, 3, 0);
    }

    /* TODO: what happens if the KNX device receives some data before 
     * the *only* _get_status() is called?
     */
    bool _check_status() {
        char buf = 0x02;
        _serial.writeBlock(&buf, 1, 0);

        do {
            ssize_t n = _serial.readBlock(&buf, 1, 0);
            if(n != 1) 
                return false;

        /* Maybe there are pending 'Reset OK' */
        } while(buf == 0x03 || buf == 0x00); 

        return (buf == 0x07);
    }

    USART_TypeDef *_port;
    std::deque<uint8_t> _stream;
    std::queue<telegram> _output;
    std::queue<telegram> _pkts;
    STM32Serial _serial;
    RingBuffer _pktbuffer;
    FastMutex _mutex;
    const int _device;
    bool _is_packet_outgoing; 
    telegram _the_outgoing;

    TAG_DEF("MiosixUSART")
};

} /* KNX */

void _miosix_tpuart_recv_thread(void *arg) {
    KNX::MiosixTPUART *backend = reinterpret_cast<KNX::MiosixTPUART *>(arg);
    STM32Serial &_serial = backend->_thread_get_device();
    const int id = backend->_thread_get_device_id();

    while(is_class_active(id)) {
        NETBENCHMARK_START(uart_waiting);
        uint8_t buf[10];

        ssize_t m = _serial.readBlock(buf, 10, 0);

        if(m > 0 && is_class_active(id))
            backend->_thread_push_data(buf, (size_t)m);

        NETBENCHMARK_STOP(uart_waiting);
    }
}
#endif /* BACKEND_MIOSIX_TPUART_H */
