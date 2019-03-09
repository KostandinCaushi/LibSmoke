#ifndef KNX_NODECOUNTER_DUMMY_H
#define KNX_NODECOUNTER_DUMMY_H

#include "counter.h"
#include <knx/config.h>

namespace KNX
{

enum NodeCounterOpcodes {
    CMD_NODECOUNTER_THATSME = 0x10,
};

template<size_t N>
class DummyNodeCounter : public NodeCounter {
public:
    DummyNodeCounter(PKTWrapper& pkt) : NodeCounter(), _pkts(pkt), _cnt(0),
                                        _update_loops_remaining(0) { }

    ~DummyNodeCounter() { }

    bool init() {
        _myid = sKConfig.id();

        _nodes.clear();
        _nodes.insert(_myid);
        _status = COUNTER_COUNTING;
        return true;
    }

    void shutdown() {
        _status = COUNTER_OFFLINE;
    }

    bool update() {
        if(_status != COUNTER_COUNTING)
            return false;

        if(!(_cnt++ % 8))
            _pkts.write(CMD_NODECOUNTER_THATSME);

        while(_pkts.count() > 0 && _nodes.size() < N) {
            pkt_t packet;
            _pkts.read(packet);

            if(packet.cmd != CMD_NODECOUNTER_THATSME)
                continue;

            if(_nodes.find(packet.src) == _nodes.end() && _nodes.size() < N) {
                LOGP("Just found a new friend. Say hello to 0x%04x.", 
                        packet.src);
                _nodes.insert(packet.src);
            }
        }

        if(_update_loops_remaining > 0) {
            if(--_update_loops_remaining == 0)
                _status = COUNTER_COMPLETED;
        } else if(_nodes.size() == N) {
            LOG("Found all nodes. Waiting for completiong...");
            _update_loops_remaining = 64;
        }

        return true;
    }

private:
    uint16_t _myid;
    PKTWrapper& _pkts;
    uint8_t _cnt;
    uint8_t _update_loops_remaining;
    TAG_DEF("DummyNodeCounter")
};

} /* KNX */

#endif /* ifndef KNX_NODECOUNTER_DUMMY_H */
