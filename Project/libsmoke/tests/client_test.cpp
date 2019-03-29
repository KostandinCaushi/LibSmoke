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

    sleep(2);

    // Test to send msg broadcast
    KNX::telegram pkt;
    uint8_t in[15]  = { 0x60, 0x1e, 0xc3, 0x13, 0x77, 0x57, 0x89, 0xa5, 0xb7, 0xa7, 0xf5, 0x04, 0xbb, 0xf3, 0xd2 };
    if(pkt.write(in, 15)) {
        client.send(pkt);
    }

    // Get sent pkt
    KNX::telegram pkt2;
    if (!client.receive(pkt2)) {
        printf("\nNo pkt received !!\n");
    }
    KNX::telegram pkt3;
    if (!client.receive(pkt3)) {
        printf("\nNo pkt received !!\n");
    }

    return 0;
}
