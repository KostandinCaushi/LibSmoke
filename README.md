# LibSmoke
LibSmoke is a library (in C++) that allows embedded systems to communicate between each others in an efficient and secure way.

It integrates together 2 libraries :
* [AES](https://github.com/kokke/tiny-AES-c), in particular AES256_CTR, used to encrypt and decrypt pkts data;
* SKNX (Securing KNX), used to exchange pkts between different devices and to generate a shared key.

## Design & Implementation
LibSmoke is divide in 2 parts : ClientSmoke and ServerSmoke.
### ServerSmoke
It's main features are to instatiate socket connection and broadcast the received pkts from the clients.
```C
class ServerSmoke{
public:
    /**
     * Constructor of Libsmoke Server.
     * Sets _ready to false in order to say that the server is not yet initialized.
     */
    ServerSmoke() : _ready(false);
    
    /**
     * Checks if the server is already initialized.
     * If not, creates the socket with the given IP and PORT.
     *
     * @param addr - IP addr of the socket to create.
     * @param port - PORT of the socket to create.
     * @return TRUE - if socket is created with no errors and is listening.
     *         FALSE - otherwise.
     */
    bool init(const char *addr, int port);
    
    /**
     * Handles connections : registering the new ones or deleting the closed ones.
     * Also used to broadcast messages to all connected clients.
     */
    void run();
    
    /**
     * Destructor of Libsmoke Server.
     * Used to close all sockets and clear the vector of the registered clients.
     * Sets also _ready to false, so the server can be initialized again.
     */
    ~ServerSmoke();
```
### ClientSmoke
It's main features are to instatiate socket connection and broadcast the received pkts from the clients.
```C
/**
 * Class of Libsmoke Client.
 *
 * @tparam KeyAlgorithm - is the KeyExchange Algorithm needed for SKNX to generate the key.
 * @tparam PORT - is the PORT used by sockets.
 * @tparam numClients - is the #Clients connected.
 */
template<typename KeyAlgorithm, uint16_t PORT, size_t numClients>
class ClientSmoke {
public:
      /**
     * Constructor of Libsmoke Client.
     *
     * @param addr const char* - is a pointer to the IP to which socket client has to connect.
     */
    ClientSmoke(const char *addr);
    
    /**
     * Initializes _backend and sknx.
     * When sknx is ONLINE retrieves the KeyAlgorithm.
     *
     * \return     TRUE - if all initializations are completed succesfully and key is retrieved.
     *             FALSE - if there's an error on initialization or key retrieving.
     */
    bool init();
    
    /**
     * Uses pktwrapper in order to get the oldest pkt received from other clients.
     * It decrypts the body using AES_CTR with the shared key of the client.
     *
     * @param data KNX::pkt_t& - pointer to a pkt that the method uses to pass the oldest recieved one.
     * @return TRUE - if there is actually a recieved pkt in the queue of pktwrapper.
     *         FALSE - if no pkt is received.
     */
    bool receive(KNX::pkt_t &data);
    
    /**
     * Uses pktwrapper in order to send data (buf), with a specific length (len)
     * and command (cmd), to a specific destination (dest).
     * It encrypts the body using AES_CTR with the shared key of the client.
     *
     * @param dest - the destination of the data.
     * @param cmd - the command to send to the client(s).
     * @param buf - the data, if there are, to send.
     * @param len - the length of the data (can be also 0).
     */
    void send(uint16_t dest, uint8_t cmd, uint8_t *buf,
              uint16_t len);
```
NB : the user can choose the KeyExchange Algorithm from the ones already implemented and available inside the SKNX library.

## Instructions to Use
+ To use ClientSmoke add `#include "libsmoke_client.h"`
* To use ServerSmoke add `#include "libsmoke_server.h"`

NB : in the repository is available also a `tests` directory, where inside can find `client_test` and `server_test`, that show how to use the library.
