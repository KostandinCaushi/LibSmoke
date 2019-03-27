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

