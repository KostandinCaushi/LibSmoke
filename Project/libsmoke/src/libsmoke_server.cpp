#include <libsmoke_server.h>
#include <knx/common.h>
#include <common.h>

#define ADDR "127.0.0.1"
//defining #define PORT 123 unsigned int --> arises problems passing parameters in init
//other problems if we try to use the TCP_PORT in common.h since it's declared as an int

using namespace std;
TAG_DEF("Main")

int main(){
    ServerSmoke server;
    unsigned short port = 123;

    if(!server.init(ADDR, port)){
        LOG("Cannot init TCP connection.");
        exit(-1);
    }

    printf("[MAIN] Server is up and running\n");
    while(!mustStop)
        server.run();

    printf("[MAIN] Shutting down...\n");

    server.shutdown();

    printf("[MAIN] Goodbye!\n");

    return 0;
}