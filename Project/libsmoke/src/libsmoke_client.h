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
template<typename KeyAlgorithm, uint16_t PORT, size_t numClients>
class ClientSmoke {
public:

    ClientSmoke(const char *addr) :
            _backend(addr), _pktwrapper(numClients, _backend),
            _key(_pktwrapper) {}


    //TODO: comment
    bool init() {
        KNX::SKNX<KeyAlgorithm, KNX::DummyNodeCounter<numClients>> sknx(_backend, numClients);

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
    bool receive(KNX::pkt_t &data) {
        _pktwrapper.update();
        if (_pktwrapper.read(data)) {
            printf("\nReceived packet\n");
            Debug::printArray(data.getData().data(), data.getData().size());

            // Get data tmp parameters
            uint8_t *tmpBuff = data.getData().data();
            uint32_t buffSize = (uint32_t) data.getData().size();
            // Decrypt MSG
            AES_init_ctx_iv(&_ctx, _key.key(), iv);
            AES_CTR_xcrypt_buffer(&_ctx, tmpBuff, buffSize);
            printf("\nDecrypted\n");
            Debug::printArray(tmpBuff, buffSize);
            return true;
        } else {
            return false;
        }
    }


    //TODO: comment
    void send(uint16_t dest, uint8_t cmd, uint8_t *buf,
              uint16_t len) {
        printf("\nSent MSG\n");
        Debug::printArray(buf, len);

        // Encrypt MSG
        AES_init_ctx_iv(&_ctx, _key.key(), iv);
        AES_CTR_xcrypt_buffer(&_ctx, buf, len);
        printf("\nEncrypted MSG\n");
        Debug::printArray(buf, len);

        // Send MSG
        _pktwrapper.write(dest, cmd, buf, len);
        _pktwrapper.update();
    }

private:
    KNX::PKTWrapper _pktwrapper;
    KeyAlgorithm _key;
    KNX::LinuxTCP<PORT> _backend;
    struct AES_ctx _ctx;
};

#endif //LIBSMOKE_CLIENT_H
