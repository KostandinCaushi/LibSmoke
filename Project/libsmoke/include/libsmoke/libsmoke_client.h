#ifndef LIBSMOKE_CLIENT_H
#define LIBSMOKE_CLIENT_H

#include <aes.hpp>
#include <telegram.h>
#include "backend.h"

static volatile bool _stopevent = false;
static volatile uint8_t iv[16]  = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
                                    0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };

namespace KNX {

//TODO: comment
    template<typename KeyAlgorithm, uint16_t PORT>
    class ClientSmoke {
    public:
        ClientSmoke();

        //TODO: comment
        bool init(int numClients, const char *addr);

        //TODO: comment
        bool receive(telegram &data);

        //TODO: comment
        void send(telegram &data);

    private:
        KeyAlgorithm _key;
        Backend &_backend;
        struct AES_ctx ctx;
    };
} // end namespace KNX

#endif //LIBSMOKE_CLIENT_H
