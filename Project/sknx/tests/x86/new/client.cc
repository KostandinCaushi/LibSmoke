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
        uint8_t event[8] = {'0','0','0','0','7','0','1','5'};
        pkt.write(event, 8);
        tcp.broadcast(pkt);

        KNX::telegram pkt2;
        while (1) {
            if (tcp.read(pkt2)) {
                tcp.read(pkt2);
                Debug::printArray(pkt2.body(), 8);
            }
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

