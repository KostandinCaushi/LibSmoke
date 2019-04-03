#include "../src/libsmoke_client.h"

#include "connection_data.h"

TAG_DEF("Main")

int main() //int argc, char *argv[])
{
    ClientSmoke<KNX::MKAKeyExchange, TCP_PORT, CLIENTS_NUM> client(TCP_IP);

    if(!client.init()) {
        printf("Cannot init TCP connection.");
        exit(-1);
    }

    sleep(2);

    // Test to send msg broadcast
    uint8_t in[20]  = { 0x60, 0x1e, 0xc3, 0x13, 0x77, 0x57, 0x89, 0xa5, 0xb7, 0xa7, 0xf5, 0x04, 0xbb, 0xf3, 0xd2,
                    0x60, 0x1e, 0xc3, 0x13, 0x77 };

    // Send data in broadcast, dest = 0x00, CMD_PING = 0x00
    client.send(0x00, 0x00, in, 20);

    // Get sent pkt
    while (true) {
        KNX::pkt_t pkt2;
        if (client.receive(pkt2)) {
            break;
        }
    }

    return 0;
}
