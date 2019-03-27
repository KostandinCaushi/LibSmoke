#ifndef LIBSMOKE_SERVER_H
#define LIBSMOKE_SERVER_H

#include <cstdio>
#include <queue>
#include <vector>
#include <linux-tcp.h>

using std::vector;
using std::queue;

// TODO: comment
static volatile bool mustStop = false;

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
    bool init(const char *addr, int port);
    // TODO: comment
    void run();

    // TODO: comment
    ~ServerSmoke() { shutdown(); };
    // TODO: comment
    void shutdown();

    //TODO: comment
    void banner() {
        printf(
                " ********************************************************************************\n"
                " * $$\\       $$\\ $$\\        $$$$$$\\                          $$\\                 \n"
                " * $$ |      \\__|$$ |      $$  __$$\\                         $$ |                \n"
                " * $$ |      $$\\ $$$$$$$\\  $$ /  \\__|$$$$$$\\$$$$\\   $$$$$$\\  $$ |  $$\\  $$$$$$\\  \n"
                " * $$ |      $$ |$$  __$$\\ \\$$$$$$\\  $$  _$$  _$$\\ $$  __$$\\ $$ | $$  |$$  __$$\\ \n"
                " * $$ |      $$ |$$ |  $$ | \\____$$\\ $$ / $$ / $$ |$$ /  $$ |$$$$$$  / $$$$$$$$ |\n"
                " * $$ |      $$ |$$ |  $$ |$$\\   $$ |$$ | $$ | $$ |$$ |  $$ |$$  _$$<  $$   ____|\n"
                " * $$$$$$$$\\ $$ |$$$$$$$  |\\$$$$$$  |$$ | $$ | $$ |\\$$$$$$  |$$ | \\$$\\ \\$$$$$$$\\ \n"
                " * \\________|\\__|\\_______/  \\______/ \\__| \\__| \\__| \\______/ \\__|  \\__| \\_______|\n\n");
    }

private:
    // TODO: comment
    queue<vector<uint8_t>> pktQueue;
    vector<Client *> clients;
    int _sock;
    bool _ready;
    fd_set _rfds;
};

#endif //LIBSMOKE_SERVER_H