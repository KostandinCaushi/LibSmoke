#ifndef BACKEND_HH
#define BACKEND_HH

#include <knx/common.h>
#include <knx/telegram.h>

namespace KNX
{

class Backend {
    public:
        Backend() : _ready(false) { }

        virtual bool init() = 0;
        virtual bool broadcast(const telegram& data) = 0;

        virtual bool read(telegram& data) = 0;
        virtual void pop() = 0;

        virtual size_t count() const = 0;
        virtual void shutdown() = 0;
        virtual bool must_update() const = 0;
        virtual bool update() = 0;
        bool isReady() const { return _ready; }
    protected:
        bool _ready;
};

} // KNX

#ifdef _MIOSIX
using miosix::FastMutex;
/* 3 Usart ports are hardcoded inside miosix kernel */
volatile bool _miosix_usart_locked[3] = {false};
FastMutex _usart_mutex; /* This one is referred to the array above */

/* Check if the class is destroyed */
bool is_class_active(int id) {
    miosix::Lock<miosix::FastMutex> l(_usart_mutex);
    return _miosix_usart_locked[id];
}

#endif

#endif /* ifndef BACKEND_HH */
