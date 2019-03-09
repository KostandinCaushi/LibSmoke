#include "common.h"
#include <stdio.h>
#include <knx/knx.h>
#include <knx/backend/linux-tpuart.h>
#include <knx/crypto/mka.h>
#include <knx/crypto/bd1.h>
#include <knx/crypto/bd2.h>
#include <knx/crypto/gdh2.h>
#include <knx/nodecounter/dummy.h>
#include <knx/pktwrapper.h>
#include <knx/crypto/policies.h>

using namespace std;
TAG_DEF("Main")

int main(int argc, char *argv[])
{
    if(argc != 2) {
        LOGP("Usage: %s <UART device>", argv[0]);
        return 1;
    }

    KNX::LinuxTPUART uart(argv[1]);
    KNX::SKNX< KNX::GDH2KeyExchange, KNX::DummyNodeCounter<2> > sknx(uart, 7);

    if(!uart.init()) {
        LOG("Cannot init UART.");
        return 0;
    }

    BENCHMARK_INIT();
    ALLBENCHMARK_START(total_exchange_time);
    if(!sknx.init()) {
        LOG("Cannot init SKNX.");
    }

    LOG("Running...");
    while(sknx.status() != KNX::SKNX_ONLINE && 
          sknx.status() != KNX::SKNX_OFFLINE) {
        sknx.update();
        usleep(50000);
    }

    if(sknx.status() == KNX::SKNX_OFFLINE) 
        LOG("SKNX Error :(");
    else {
        LOG("Key exchange completed.");
        sknx.update();
    }
    ALLBENCHMARK_STOP(total_exchange_time);

    BENCHMARK_PRINT();

    uart.shutdown();
    return 0;
}

