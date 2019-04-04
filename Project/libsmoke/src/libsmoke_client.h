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

/**
 * It's a fixed initialization vector, used to initialize AES
 */
static const uint8_t iv[16]  = { 0xc0, 0xa1, 0x62, 0xb3, 0x84, 0x25, 0xe6,
                                    0xc7, 0x58, 0x99, 0xaa, 0x0b, 0x1c, 0x6d, 0xbe, 0xff };


/**
 * Class of Libsmoke Client.
 *
 * @tparam KeyAlgorithm - is the KeyExchange Algorithm needed for SKNX to generate the key.
 * @tparam PORT - is the PORT used by sockets.
 * @tparam numClients - is the #Clients connected.
 */
template<typename KeyAlgorithm, uint16_t PORT, size_t numClients>
class ClientSmoke {
public:

    /**
     * Constructor of Libsmoke Client.
     *
     * @param addr const char* - is a pointer to the IP to which socket client has to connect.
     */
    ClientSmoke(const char *addr) :
            _backend(addr), _pktwrapper(numClients, _backend),
            _key(_pktwrapper) {}


    /**
     * Initializes _backend and sknx.
     * When sknx is ONLINE retrieves the KeyAlgorithm.
     *
     * \return     TRUE - if the all initializations are completed succesfully and key is retrieved.
     *             FALSE - if there's an error on initializations or key retrieving.
     */
    bool init() {
        // Instantiate SKNX
        KNX::SKNX<KeyAlgorithm, KNX::DummyNodeCounter<numClients>> sknx(_backend, numClients);

        // Initialize backend
        if (!_backend.init()) {
            printf("Cannot init TCP connection.");
            return false;
        }

        // Initialize sknx
        if (!sknx.init()) {
            printf("Cannot init SKNX.");
            return false;
        }

        printf("Running....");
        // Starts the key exchange
        while ((sknx.status() != KNX::SKNX_ONLINE &&
                sknx.status() != KNX::SKNX_OFFLINE)) {
            sknx.update();
            usleep(50000);
        }

        // Check final state
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


    /**
     * Uses pktwrapper in order to get the oldest pkt received from other clients.
     * It decrypts the body using AES_CTR with the shared key of the client.
     *
     * @param data KNX::pkt_t& - pointer to a pkt that the method uses to pass the oldest recieved one.
     * @return TRUE - if there is actually a recieved pkt in the queue of pktwrapper.
     *          FALSE - if no pkt is received.
     */
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


    /**
     * Uses pktwrapper in order to send data (buf), with a specific length (len)
     * and command (cmd), to a specific destination (dest).
     * It encrypts the body using AES_CTR with the shared key of the client.
     *
     * @param dest - the destination of the data.
     * @param cmd - the command to send to the client(s).
     * @param buf - the data, if there are, to send.
     * @param len - the length of the data (can be also 0).
     */
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
    /**
     * Used in order to send and receive pkts.
     */
    KNX::PKTWrapper _pktwrapper;
    /**
     * Used to save locally the shared key.
     */
    KeyAlgorithm _key;
    /**
     * Used to save locally the LinuxTCP backend connection used to communicate.
     */
    KNX::LinuxTCP<PORT> _backend;
    /**
     * Used to save locally an instance of AES.
     */
    struct AES_ctx _ctx;
};

#endif //LIBSMOKE_CLIENT_H
