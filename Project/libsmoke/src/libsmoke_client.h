#ifndef LIBSMOKE_CLIENT_H
#define LIBSMOKE_CLIENT_H

#include <tiny-AES-c-master/aes.hpp>
#include <sknx/src/shared/knx/knx.h>
#include <sknx/src/shared/knx/backend/linux-tcp.h>
#include <sknx/src/shared/knx/nodecounter/dummy.h>
#include <sknx/src/shared/knx/crypto/mka.h>
#include <sknx/src/shared/knx/pktwrapper.h>
#include <sknx/src/shared/knx/backend/backend.h>
#include <sknx/src/shared/knx/debug.h>


static volatile bool _stopevent = false;
static const uint8_t iv[16]  = { 0xc0, 0xa1, 0x62, 0xb3, 0x84, 0x25, 0xe6,
                                    0xc7, 0x58, 0x99, 0xaa, 0x0b, 0x1c, 0x6d, 0xbe, 0xff };


//TODO: comment
template<typename KeyAlgorithm, uint16_t PORT>
class ClientSmoke {
public:

    ClientSmoke(int numClients, const char *addr) :
            _backend(addr), _pktwrapper(numClients, _backend),
            _key(_pktwrapper) {}


    //TODO: comment
    bool init() {
        KNX::SKNX<KeyAlgorithm, KNX::DummyNodeCounter<2> > sknx(_backend, 2);

        if (!_backend.init()) {
            printf("Cannot init TCP connection.");
            return false;
        }

        if (!sknx.init()) {
            printf("Cannot init SKNX.");
            return false;
        }

        printf("Running....");
        while (!_stopevent && (sknx.status() != KNX::SKNX_ONLINE &&
                               sknx.status() != KNX::SKNX_OFFLINE)) {
            sknx.update();
            usleep(50000);
        }

        if (sknx.status() == KNX::SKNX_OFFLINE) {
            printf("SKNX Error :(");
            return false;
        } else if (sknx.status() == KNX::SKNX_ONLINE) {
            printf("Key exchange completed.\n");

            // retrieve the calculated key
            if (sknx.getKey(_key)) {
                // used to debug the key
                Debug::printArray(_key.key(), _key.size());
                return true;
            } else {
                return false;
            }
        }
    }


    //TODO: comment
    bool receive(KNX::telegram &data) {
        _backend.update();
        if (_backend.read(data)) {
            printf("\nReceived packet\n");
            Debug::printArray(data.body(), 15);

            // Decrypt MSG
            AES_init_ctx_iv(&_ctx, _key.key(), iv);
            AES_CTR_xcrypt_buffer(&_ctx, data.body(), 15);
            printf("\nDecrypted\n");
            Debug::printArray(data.body(), 15);
            return true;
        } else {
            return false;
        }
    }


    //TODO: comment
    void send(KNX::telegram &data) {
        printf("\nSent MSG\n");
        Debug::printArray(data.body(), 15);

        // Encrypt MSG
        AES_init_ctx_iv(&_ctx, _key.key(), iv);
        AES_CTR_xcrypt_buffer(&_ctx, data.body(), 15);
        printf("\nEncrypted MSG\n");
        Debug::printArray(data.body(), 15);

        // Send MSG
        _backend.broadcast(data);
    }

private:
    KNX::PKTWrapper _pktwrapper;
    KeyAlgorithm _key;
    KNX::LinuxTCP<PORT> _backend;
    struct AES_ctx _ctx;
};

#endif //LIBSMOKE_CLIENT_H
