#ifndef KNX_KNX_H
#define KNX_KNX_H

#include "common.h"
#include "backend/backend.h"
#include "pktwrapper.h"
#include "nodecounter/counter.h"
#include "crypto/crypto.h"

namespace KNX
{

enum SKNXStatus {
    SKNX_OFFLINE,
    SKNX_COUNTING,
    SKNX_HANDSHAKING,
    SKNX_FINALIZING,
    SKNX_ONLINE
};

struct Node {
    uint16_t id;
};

template <typename KeyAlgorithm, typename NodeCountStrategy>
class SKNX {
public:
    SKNX(Backend& backend, size_t max_clients) : 
        _pktwrapper(max_clients, backend), _counter(_pktwrapper), 
        _key(_pktwrapper), _status(SKNX_OFFLINE), _backend(backend) {}

    bool init() {
        if(_status != SKNX_OFFLINE)
            return false;

        CTRBENCHMARK_START(counter_init);
        if(!_counter.init())
            return false;
        CTRBENCHMARK_STOP(counter_init);

        _status = SKNX_COUNTING;
        CTRBENCHMARK_START(counter_counting);
        return true;
    }

    bool update() {
        _pktwrapper.update();
        switch(_status) {
            case SKNX_OFFLINE:
                return false;
            case SKNX_COUNTING:
                return _update_counter();
            case SKNX_HANDSHAKING:
                return _update_handshaking();
            case SKNX_FINALIZING:
                if(_backend.must_update())
                    _backend.update();
                else
                    _status = SKNX_ONLINE;
                break;
            case SKNX_ONLINE:
                LOG("KEY EXCHANGE OK");
                Debug::printArray(_key.key(), _key.size());
                return true; 
        }
        return false;
    }

    SKNXStatus status() const { return _status; }

    const uint8_t * getKey(){
        return _key.key();
    }

    /*
    virtual bool send(const uint8_t *data, size_t len) {
        return false; 
    }

    virtual ssize_t read(uint8_t *data, size_t len) {
        return -1;
    }
    */

private:
    PKTWrapper _pktwrapper;
    NodeCountStrategy _counter;
    KeyAlgorithm _key;
    SKNXStatus _status;
    Backend &_backend;

    bool _update_counter() {
        switch(_counter.status()) {
            case COUNTER_OFFLINE:
                _status = SKNX_OFFLINE;
                return false;
            case COUNTER_COUNTING:
                if(!_counter.update()) {
                    CTRBENCHMARK_STOP(counter_counting);
                    _status = SKNX_OFFLINE;
                    return false;
                }
                return true;
            case COUNTER_COMPLETED:
                CTRBENCHMARK_STOP(counter_counting);
                // Start key exchanging
                const nodeset_t& count = _counter.nodes();
                _counter.shutdown();
                LOGP("Counting completed (%lu clients).", count.size());
                LOG("Handshaking with my new friends..");
                ALLBENCHMARK_START(keyexchange_running);
                if(!_key.init(count)) {
                    _status = SKNX_OFFLINE;
                    return false;
                }
                _status = SKNX_HANDSHAKING;
                return true;
        }
        return false;
    }

    bool _update_handshaking() {
        switch(_key.status()) {
            case KEYEXCHANGE_OFFLINE:
                _status = SKNX_OFFLINE;
                return false;
            case KEYEXCHANGE_ERROR:
                ALLBENCHMARK_STOP(keyexchange_running);
                _status = SKNX_OFFLINE;
                _key.shutdown();
                return false;
            case KEYEXCHANGE_RUNNING:
                if(!_key.update()) {
                    _status = SKNX_OFFLINE;
                    _key.shutdown();
                }
                return true;
            case KEYEXCHANGE_COMPLETED:
                ALLBENCHMARK_STOP(keyexchange_running);
                _status = SKNX_FINALIZING;
                return true;
        }
        return false;
    }

    TAG_DEF("KNX")
};

} /* KNX */

#endif /* ifndef KNX_KNX_H */
