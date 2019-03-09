#ifndef KNX_CRYPTO_BD2_H
#define KNX_CRYPTO_BD2_H

#include "crypto.h"
#include "policies.h"
#include "bd2_utils.h"

namespace KNX
{

template<typename ChairChooser = Policies::LowestIdNodeChooser>
class BD2CustomKeyExchange : public KeyExchange, private ChairChooser {

public:
    BD2CustomKeyExchange(PKTWrapper& pkt) : KeyExchange(), _pkts(pkt) { 
        _behavior = NULL;
    }

    ~BD2CustomKeyExchange() {
        shutdown();
    }

    bool init(const nodeset_t& nodes) {
        if(_status != KEYEXCHANGE_OFFLINE)
            return false;

        if(nodes.size() > 255 || nodes.size() < 2) // I'm using uint8_t!
            return false;

        _status = KEYEXCHANGE_RUNNING;
        _nodes = nodes; 
        size_t chair_id = this->get_node(nodes);

        if(!sKConfig.ctr_drbg_available()) {
            _status = KEYEXCHANGE_ERROR;
            return false;
        }

        if(_behavior) {
            delete _behavior;
            _behavior = NULL;
        }

        if(sKConfig.id() == chair_id) {
            LOGP("I'm the root node, %04x!", chair_id);
            _behavior = new BD2::ChairBehavior(_pkts, nodes);
        } else {
            LOGP("I'm a simple node (%04x). %04x is the root.", 
                sKConfig.id(), chair_id);
            _behavior = new BD2::NodeBehavior(_pkts, chair_id);
        }

        if(!_behavior->init()) {
            LOG("_behavior->init() failed.");
            delete _behavior;
            _behavior = NULL;
            return false;
        }

        return true;
    }

    void shutdown() {
        _status = KEYEXCHANGE_OFFLINE;

        if(_behavior) {
            delete _behavior;
            _behavior = NULL;
        }
    }

    bool update() {
        if(_status != KEYEXCHANGE_RUNNING) 
            return false;

        while(_pkts.count() > 0) {
            pkt_t packet;
            _pkts.read(packet);

            if(_behavior && !_behavior->on_recv(packet))
                return false;
        }

        if(_behavior && _behavior->completed())
            _status = KEYEXCHANGE_COMPLETED;

        return true;
    }

    const uint8_t * key() const { 
        if(!_behavior)
            return NULL;
        return _behavior->key();
    }

    size_t size() const { 
        if(!_behavior)
            return 0;
        return _behavior->size();
    }

private:
    PKTWrapper& _pkts;

    /* Are you the king or a simple node? */
    BD2::Behavior *_behavior;

    TAG_DEF("BD2Core")
};

typedef BD2CustomKeyExchange<> BD2KeyExchange;

} /* KNX */

#endif /* ifndef KNX_CRYPTO_BD2_H */
