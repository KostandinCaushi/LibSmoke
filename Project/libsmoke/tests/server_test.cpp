#include "../src/libsmoke_server.h"
#include <sknx/src/shared/knx/debug.h>
#include <cstdlib>

#include "connection_data.h"

TAG_DEF("Main")

int main() //int argc, char *argv[])
{
    ServerSmoke server;

    if(!server.init(TCP_IP, TCP_PORT)) {
        printf("Cannot init TCP connection.");
        exit(-1);
    }

    printf("[MAIN] Server is up and running\n");
    while(!mustStop) {
        server.run();
    }

    printf("[MAIN] Shutting down.. \n");
    server.shutdown();
    printf("[MAIN] Goodbye\n");
    return 0;
}