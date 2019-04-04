#ifndef LIBSMOKE_SERVER_H
#define LIBSMOKE_SERVER_H

#include <cstdio>
#include <queue>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSZ 512

using std::vector;
using std::queue;

/**
 * Used in case of errors to stop the server.
 */
static volatile bool mustStop = false;
/**
 * Used to save all connected Clients.
 */
struct Client {
    int sock;
    struct sockaddr_in in;
    bool mustDelete;

    Client() : sock(0), mustDelete(false) {}
};

/**
 * Class of Libsmoke Server.
 */
class ServerSmoke{
public:

    /**
     * Constructor of Libsmoke Server.
     * Sets _ready to false in order to say that the server is not yet initialized.
     */
    ServerSmoke() : _ready(false) {}


    /**
     * Checks if the server is already initialized.
     * If not creates the socket with the given IP and PORT.
     *
     * @param addr - IP addr of the socket to create.
     * @param port - PORT of the socket to create.
     * @return TRUE - if socket is created with no errore and is listening.
     *          FALSE - otherwise.
     */
    bool init(const char *addr, int port) {
        if(_ready)
            return false;

        // Set socket parameters
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

        // Socket ready and listening
        _ready = true;
        banner();
        return true;
    }


    /**
     * Handles connections : registering the new ones or deleting the closed ones.
     * Also used to broadcast messages to all connected clients.
     */
    void run() {
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


    /**
     * Destructor of Libsmoke Client.
     * It just calls the shutdown() method.
     */
    ~ServerSmoke() {
        shutdown();
    }


    /**
     * Used to close all sockets and clear the vector of the registered clients.
     * Sets also _ready to false, so the server can be initialized again.
     */
    void shutdown() {
        _ready = false;
        close(_sock);
        for(Client* c : clients) {
            close(c->sock);
            delete c;
        }
        clients.clear();
    }


    /**
     * Banner displayed when the server initialization is completed
     * and it's waiting for clients to connect.
     */
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
    /**
     * Used to save the pkts that has to be broadcasted.
     */
    queue<vector<uint8_t>> pktQueue;
    /**
     * Used to save the connected clients.
     */
    vector<Client *> clients;
    /**
     * Used to save the listening socket of the server.
     */
    int _sock;
    /**
     * Used to check if the server is already initialized or not.
     */
    bool _ready;
    /**
     * Selection Function used for socket.
     */
    fd_set _rfds;
};

#endif //LIBSMOKE_SERVER_H