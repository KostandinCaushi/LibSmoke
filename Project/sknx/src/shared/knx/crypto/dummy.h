#ifndef KNX_CRYPTO_DUMMY_DO_NOT_USE
#define KNX_CRYPTO_DUMMY_DO_NOT_USE

#include "crypto.h"

namespace KNX
{

template<size_t KeyLength>
class DummyKeyExchange : public KeyExchange {

public:
    DummyKeyExchange(Backend& backend) : KeyExchange(), backend(backend) {}

    bool init(size_t nodes) {
        LOG("This class is intended ONLY for testing." 
            "Do *NOT* use in a production environment.");
        _nodes = nodes;
        _status = KEYEXCHANGE_COMPLETED;
        return true;
    }

    void shutdown() {
        _status = KEYEXCHANGE_OFFLINE;
    }

    bool update() {
        return true;
    }

    bool key(uint8_t *key, size_t len) const {
        if(_status == KEYEXCHANGE_COMPLETED && len >= KeyLength) {
            memset(key, 'A', KeyLength);
            return true;
        }

        return false;
    }

    size_t size() const {
        return KeyLength;
    }

    size_t dummyGetNodes() { return _nodes; }
    Backend& dummyGetBackend() { return backend; }
private:
    size_t _nodes;
    Backend& backend;
    TAG_DEF("DummyKeyExchange")
};

} /* KNX */

#endif /* ifndef KNX_CRYPTO_DUMMY_DO_NOT_USE */
