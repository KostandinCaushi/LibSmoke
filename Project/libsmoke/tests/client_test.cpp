#include <libsmoke_client.h>
#include <shared/knx/debug.h>
#include <cstdlib>
#include <shared/knx/crypto/mka.h>

#include "connection_data.h"

TAG_DEF("Main")

int main() //int argc, char *argv[])
{
    KNX::ClientSmoke<KNX::MKAKeyExchange, TCP_PORT> client;

    if(!client.init(2, TCP_IP)) {
        printf("Cannot init TCP connection.");
        exit(-1);
    }

    // TODO: send pkts

    return 0;
}
