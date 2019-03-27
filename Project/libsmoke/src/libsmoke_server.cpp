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


//TODO: comment
void ServerSmoke::run() {
    char buf[BUFSZ];
    if(!_ready) return;
    int max_id = _sock;

    FD_ZERO(&_rfds);
    FD_SET(_sock, &_rfds);

    // Register all sockets
    for(size_t i=0; i < clients.size(); i++) {
        Client *c = clients[i];
        if(c->sock == -1) {
            c->mustDelete = true;
            continue;
        }

        max_id = (max_id > c->sock) ? max_id : c->sock;
        FD_SET(c->sock,  &_rfds);
    }


    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    int n = select(max_id + 1, &_rfds, NULL, NULL, &tv);

    if(n > 0) {
        // Handle new connections
        if(FD_ISSET(_sock, &_rfds)) {
            Client *c = new Client();

            socklen_t len = sizeof(c->in);
            c->sock = accept(_sock, (struct sockaddr *) &(c->in), &len);

            if(c->sock < 0) {
                fprintf(stderr,
                        "[ERROR] Cannot accept a new connection\n");
                delete c;
            }

            printf("[NEWCONN] Say welcome to %s:%d\n",
                   inet_ntoa(c->in.sin_addr), c->in.sin_port);

            clients.push_back(c);
        }

        for(size_t i = 0; i < clients.size(); i++) {
            Client *c = clients[i];
            if(c->mustDelete) continue;
            if(FD_ISSET(c->sock, &_rfds)) {
                int m = recv(c->sock, buf, BUFSZ, 0);

                // Connection closed. Delete the socket
                if(m <= 0) {
                    c->mustDelete = true;
                } else if(m > 0) {
                    printf("[MSGPUSH] Broadcasting data from %s:%d\n",
                           inet_ntoa(c->in.sin_addr), c->in.sin_port);
                    vector<uint8_t> temp(buf, buf+m);
                    pktQueue.push(temp);
                }
            }
        }

        // Broadcast messages
        while(!pktQueue.empty()) {
            vector<uint8_t> &data = pktQueue.front();
            for(size_t i = 0; i < clients.size(); i++) {
                Client *c = clients[i];
                if(c->mustDelete) continue; // Only active clients

                if(send(c->sock, (const char *) &data[0],
                        data.size(), 0) < (ssize_t)data.size())
                    c->mustDelete = true;
            }
            pktQueue.pop();
        }

        // Remove disconnected clients
        for(size_t i = 0; i < clients.size(); i++) {
            Client *c = clients[i];
            if(c->mustDelete) {
                printf("[DISCONN] Say goodbye to %s:%d\n",
                       inet_ntoa(c->in.sin_addr), c->in.sin_port);
                if(c->sock > 0) close(c->sock);
                clients.erase(clients.begin() + i--);
                delete c;
            }
        }
    }
}


//TODO: comment
void ServerSmoke::shutdown() {
    _ready = false;
    close(_sock);
    for(Client* c : clients) {
        close(c->sock);
        delete c;
    }
    clients.clear();
}