#include "common.h"
#include <signal.h>
#include <stdio.h>
#include <knx/knx.h>
#include <knx/backend/linux-tcp.h>
#include <knx/crypto/mka.h>
#include <knx/crypto/bd1.h>
#include <knx/crypto/bd2.h>
#include <knx/crypto/gdh2.h>
#include <knx/nodecounter/dummy.h>
#include <knx/pktwrapper.h>
#include <knx/crypto/policies.h>
#include <iostream>
#include <knx/telegram.h>
#include <aes.hpp>

static volatile bool _stopevent = false;

using namespace std;
TAG_DEF("Main")

void on_signal(int s) {
    _stopevent = true;
    signal(s, on_signal);
}

int main()
{
    KNX::LinuxTCP<TCP_PORT> tcp;
    KNX::SKNX< KNX::MKAKeyExchange, KNX::DummyNodeCounter<2> > sknx(tcp, 2);

    if(!tcp.init()) {
        LOG("Cannot init TCP connection.");
        return 0;
    }

    BENCHMARK_INIT();
    ALLBENCHMARK_START(total_exchange_time);
    if(!sknx.init()) {
        LOG("Cannot init SKNX.");
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGQUIT, on_signal);

    LOG("Running...");
    while(!_stopevent && (sknx.status() != KNX::SKNX_ONLINE && 
          sknx.status() != KNX::SKNX_OFFLINE)) {
        sknx.update();
        usleep(50000);
    }

    if(sknx.status() == KNX::SKNX_OFFLINE) 
        LOG("SKNX Error :(");
    else if(sknx.status() == KNX::SKNX_ONLINE) {
        LOG("Key exchange completed.");

        // retrieve the calculated key
        KNX::MKAKeyExchange _key = sknx.getKey();
        // used to debug the key
        Debug::printArray(_key.key(), _key.size());

        // TODO : manda messaggio cryptato
        // Test to send msg broadcast
        KNX::telegram pkt;
        uint8_t in[15]  = { 0x60, 0x1e, 0xc3, 0x13, 0x77, 0x57, 0x89, 0xa5, 0xb7, 0xa7, 0xf5, 0x04, 0xbb, 0xf3, 0xd2 };
        uint8_t iv[16]  = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };
        printf("Initial MSG\n");
        Debug::printArray(in, 15);
        // instatiated AES
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, _key.key(), iv);
        AES_CTR_xcrypt_buffer(&ctx, in, 15);
        printf("Encrypted MSG\n");
        Debug::printArray(in, 15);

        // Send pkt
        bool sent = 0;

        for (int (i) = 0; (i) < 5 ; ++(i)) {
            while(sent!=1){
                if(pkt.write(in, 15)) {
                    printf("Written packet %d\n", (i+1));
                    tcp.broadcast(pkt);
                    sent = 1;
                }

            }

            KNX::telegram pkt2;
            while (sent==1) {
                tcp.update();
                if (tcp.read(pkt2)) {
                    printf("Recieved packet %d\n", (i+1));
                    sent = 0;
                    Debug::printArray(pkt2.body(), 15);
                    AES_init_ctx_iv(&ctx, _key.key(), iv);
                    AES_CTR_xcrypt_buffer(&ctx, pkt2.body(), 15);
                    printf("Decrypted \n");
                    Debug::printArray(pkt2.body(), 15);
                }
            }

            sleep(5);
        }

    }
    ALLBENCHMARK_STOP(total_exchange_time);

    BENCHMARK_PRINT();

    tcp.shutdown();

    signal(SIGINT, 0);
    signal(SIGTERM, 0);
    signal(SIGQUIT, 0);
    return 0;
}

