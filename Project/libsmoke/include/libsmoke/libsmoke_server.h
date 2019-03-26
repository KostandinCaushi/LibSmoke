#ifndef LIBSMOKE_SERVER_H
#define LIBSMOKE_SERVER_H

#include <cstdio>
#include <queue>
#include <vector>
#include <linux-tcp.h>


#define VERSION "0.01"
#define BUFSZ 512

// TODO: comment
static volatile bool mustStop = false;

// TODO: comment
struct Client {
    int sock;
    struct sockaddr_in in;
    bool mustDelete;

    Client() : sock(0), mustDelete(false) {}
};

class ServerSmoke{
public:

    // TODO: comment
    ServerSmoke() : _ready(false) {};

    // TODO: comment
    bool init(const char *addr, unsigned short &port){};
    // TODO: comment
    void run() {};

    // TODO: comment
    ~ServerSmoke() {};
    // TODO: comment
    void shutdown() {};

private:
    // TODO: comment
    queue<vector<uint8_t>> pktQueue;
    vector<Client *> clients;
    int _sock;
    bool _ready;
    fd_set _rfds;
};

#endif //LIBSMOKE_SERVER_H