#include "../src/libsmoke_client.h"

#include "connection_data.h"

TAG_DEF("Main")

int main() //int argc, char *argv[])
{
    ClientSmoke<KNX::MKAKeyExchange, TCP_PORT> client(CLIENTS_NUM, TCP_IP);

    if(!client.init()) {
        printf("Cannot init TCP connection.");
        exit(-1);
    }

    // TODO: send pkts

    return 0;
}
