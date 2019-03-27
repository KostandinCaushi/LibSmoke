#include <libsmoke_client.h>
#include <shared/knx/backend/linux-tcp.h>
#include <shared/knx/nodecounter/dummy.h>
#include <shared/knx/knx.h>

namespace KNX {

//TODO: comment
    template<typename KeyAlgorithm, uint16_t PORT>
    bool ClientSmoke<KeyAlgorithm, PORT>::init(int numClients, const char *addr) {

        LinuxTCP<PORT> tcp(addr);
        SKNX<KeyAlgorithm, KNX::DummyNodeCounter<2> > sknx(tcp, numClients);

        if (!tcp.init()) {
            printf("Cannot init TCP connection.");
            return false;
        }
        _backend = tcp;

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
            printf("Key exchange completed.");

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
    template<typename KeyAlgorithm, uint16_t PORT>
    bool ClientSmoke<KeyAlgorithm, PORT>::receive(KNX::telegram &data) {

        _backend.update();
        if (_backend.read(data)) {
            printf("Received packet\n");
            Debug::printArray(data.body(), 15);

            // Decrypt MSG
            AES_init_ctx_iv(&ctx, _key.key(), iv);
            AES_CTR_xcrypt_buffer(&ctx, data.body(), 15);
            printf("Decrypted \n");
            Debug::printArray(data.body(), 15);
            return true;
        } else {
            return false;
        }
    }

//TODO: comment
    template<typename KeyAlgorithm, uint16_t PORT>
    void ClientSmoke<KeyAlgorithm, PORT>::send(KNX::telegram &data) {

        printf("Sent MSG\n");
        Debug::printArray(data.body(), 15);

        // Encrypt MSG
        AES_init_ctx_iv(&ctx, _key.key(), iv);
        AES_CTR_xcrypt_buffer(&ctx, data.body(), 15);
        printf("Encrypted MSG\n");
        Debug::printArray(data.body(), 15);

        // Send MSG
        _backend.broadcast(data);
    }
}