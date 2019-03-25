#ifndef KNX_CRYPTO_GDH2_H
#define KNX_CRYPTO_GDH2_H

#include "crypto.h"
#include "gdh2_utils.h"

namespace KNX
{

class GDH2KeyExchange : public KeyExchange {
public:
    GDH2KeyExchange(PKTWrapper& pkt) : KeyExchange(), _pkts(pkt) {
        _behavior = NULL;
    }

    ~GDH2KeyExchange() {
        shutdown();
    }

    bool init(const nodeset_t& nodes) {
        if(_status != KEYEXCHANGE_OFFLINE)
            return false;

        if(nodes.size() > 255 || nodes.size() < 2) // I'm using uint8_t!
            return false;

        _status = KEYEXCHANGE_RUNNING;
        _nodes = nodes; 

        if(!sKConfig.ctr_drbg_available()) {
            _status = KEYEXCHANGE_ERROR;
            return false;
        }

        GDH2::network_t net(nodes);

        if(_behavior) {
            delete _behavior;
            _behavior = NULL;
        }

        if(net.id == net.first) {
            LOGP("I'm the root node, %04x!", net.id);
            _behavior = new GDH2::FirstNodeBehavior(_pkts, nodes, net);
        } else if(net.id == net.last) {
            LOGP("I'm the last node, %04x!", net.id);
            _behavior = new GDH2::LastNodeBehavior(_pkts, nodes, net);
        } else {
            LOGP("I'm a simple node (Last: %04x, Next: %04x).", 
                    net.last, net.next);
            _behavior = new GDH2::SimpleNodeBehavior(_pkts, nodes, net);
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
    GDH2::Behavior *_behavior;

    TAG_DEF("GDH2Core")
};

} /* KNX */

#endif /* ifndef KNX_CRYPTO_GDH2_H */
