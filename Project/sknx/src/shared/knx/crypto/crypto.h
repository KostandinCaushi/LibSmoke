#ifndef KNX_CRYPTO_CRYPTO_H
#define KNX_CRYPTO_CRYPTO_H

namespace KNX 
{

enum KeyExchangeStatus {
    KEYEXCHANGE_OFFLINE,
    KEYEXCHANGE_ERROR,
    KEYEXCHANGE_RUNNING,
    KEYEXCHANGE_COMPLETED
};

#define KEYSIZE 66
class KeyExchange {
    public:
        KeyExchange() : _status(KEYEXCHANGE_OFFLINE) {}

        virtual bool init(const nodeset_t& nodes) = 0;
        virtual void shutdown() = 0;
        virtual bool update() = 0;

        virtual const uint8_t * key() const = 0;
        virtual size_t size() const = 0;

        KeyExchangeStatus status() const { return _status; }

    protected:
        KeyExchangeStatus _status;
        nodeset_t _nodes;
};

} /* KNX */

#endif /* ifndef KNX_CRYPTO_CRYPTO_H */
