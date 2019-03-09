#ifndef BACKEND_MIOSIX_USART_H
#define BACKEND_MIOSIX_USART_H

#ifndef _MIOSIX
#error "This backend is available only using Miosix."
#endif

#include <miosix.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/sync.h>
#include <drivers/serial_stm32.h>
#include <vector>
#include <queue>
#include <memory>

#include "backend.h"
#include <knx/ringbuffer.h>

#define UART_RINGBUFSIZE 256

using miosix::Lock;
using miosix::FastMutex;
using miosix::Thread;
using miosix::STM32Serial;

void _miosix_usart_recv_thread(void *arg);

namespace KNX {

class MiosixUSART : public Backend {
public:
    MiosixUSART(int device, int baudrate) : 
        Backend(), _pktbuffer(UART_RINGBUFSIZE), _remaining(0), _running(false),
        _serial(device, baudrate), _device(device - 1) { }

    ~MiosixUSART() {
        shutdown();
    }

    bool init() {
        Lock<FastMutex> l(_usart_mutex);

        if(_device < 0 || _device >= (ssize_t) sizeof(_miosix_usart_locked)) {
            LOGP("Cannot initialize an invalid USART device %d", _device);
            return false;
        }

        if(_ready || _miosix_usart_locked[_device])
            return false;

        _miosix_usart_locked[_device] = true;
        _ready = true;
        _running = true;

        Thread::create(_miosix_usart_recv_thread, 1024, 
                miosix::MAIN_PRIORITY,reinterpret_cast<void *>(this));

        return true;
    }

    bool broadcast(const telegram &data) {
        if(!_ready || !_running)
            return false;

        return _serial.writeBlock(data.raw(), data.size(), 0);
    }

    bool must_update() const {
        return false;
    }

    bool read(telegram &data) {
        if(!_ready || !_running || _pkts.size() == 0)
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
        _running = false;

        if(_device < 0 || _device >= (ssize_t) sizeof(_miosix_usart_locked))
            return;

        _miosix_usart_locked[_device] = false;
    }

    bool update() {
        Lock<FastMutex> l(_mutex);

        if(!_ready || !_running) 
            return false;

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

    std::vector<uint8_t> _stream;
    std::queue<telegram> _pkts;
    RingBuffer _pktbuffer;
    size_t _remaining;
    bool _running;
    STM32Serial _serial;
    FastMutex _mutex;
    const int _device;

    TAG_DEF("MiosixUSART")
};

} /* KNX */

void _miosix_usart_recv_thread(void *arg) {
    KNX::MiosixUSART *backend = reinterpret_cast<KNX::MiosixUSART *>(arg);
    STM32Serial &_serial = backend->_thread_get_device();
    const int id = backend->_thread_get_device_id();

    while(is_class_active(id)) {
        NETBENCHMARK_START(uart_waiting);
        uint8_t buf[128];

        ssize_t m = _serial.readBlock(buf, 128, 0);

        if(m > 0 && is_class_active(id))
            backend->_thread_push_data(buf, (size_t)m);

        NETBENCHMARK_STOP(uart_waiting);
    }
}
#endif /* BACKEND_MIOSIX_USART_H */
