#ifndef KNX_NODECOUNTER_COUNTER_H
#define KNX_NODECOUNTER_COUNTER_H

#include <knx/common.h>

namespace KNX {

enum NodeCounterStatus {
    COUNTER_OFFLINE,
    COUNTER_COUNTING,
    COUNTER_COMPLETED
};

typedef std::set<uint16_t> nodeset_t;

class NodeCounter {
public:
    NodeCounter() : _status(COUNTER_OFFLINE) {}

    virtual bool init() = 0;
    virtual void shutdown() = 0;
    virtual bool update() = 0;

    NodeCounterStatus status() const { return _status; }
    const std::set<uint16_t> &nodes() const { return _nodes; }
    size_t count() const { return _nodes.size(); }

protected:
    NodeCounterStatus _status;
    nodeset_t _nodes;
};

}

#endif /* ifndef NODECOUNTER_COUNTER_H */
