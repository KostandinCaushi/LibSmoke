#include <libsmoke_server.h>

#define BUFSZ 512


//TODO: comment
bool ServerSmoke::init(const char *addr, int port) {
    if(_ready)
        return false;

    struct sockaddr_in in;
    in.sin_family = AF_INET;
    in.sin_port = htons(port);
    in.sin_addr.s_addr = inet_addr(addr);

    _sock = socket(AF_INET, SOCK_STREAM, 0);
    if(_sock < 0) {
        fprintf(stderr, "Cannot create server socket\n");
        _ready = false;
        return false;
    }

    if(bind(_sock, (struct sockaddr *)&in, sizeof(in)) < 0) {
        fprintf(stderr, "Cannot bind on port %u\n", port);
        close(_sock);
        _ready = false;
        return false;
    }

    if(listen(_sock, 10) < 0) {
        fprintf(stderr, "Cannot start listening\n");
        close(_sock);
        _ready = false;
        return false;
    }

    _ready = true;
    return true;
}


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