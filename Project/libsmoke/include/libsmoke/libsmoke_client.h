#ifndef LIBSMOKE_CLIENT_H
#define LIBSMOKE_CLIENT_H

#include <aes.hpp>
#include <knx/knx.h>
#include <shared/knx/backend/linux-tcp.h>
#include <shared/knx/nodecounter/dummy.h>
#include <knx/crypto/mka.h>
#include <shared/knx/debug.h>


static volatile bool _stopevent = false;
static volatile uint8_t iv[16]  = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
                                    0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };


//TODO: comment
template<typename KeyAlgorithm, uint16_t PORT>
class ClientSmoke {
public:
    ClientSmoke(int numClients, const char *addr);

    //TODO: comment
    bool init();

    //TODO: comment
    bool receive(KNX::telegram &data);

    //TODO: comment
    void send(KNX::telegram &data);

private:
    KeyAlgorithm _key;
    KNX::Backend &_backend;
    struct AES_ctx _ctx;
};

#endif //LIBSMOKE_CLIENT_H
