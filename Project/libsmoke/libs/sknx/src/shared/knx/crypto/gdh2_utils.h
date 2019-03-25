#ifndef KNX_CRYPTO_GDH2_UTILS_H
#define KNX_CRYPTO_GDH2_UTILS_H

#include <knx/common.h>

extern "C" {
#include <sban/gdh2.h>
}

#define GDH2_BUFSIZE 66
namespace KNX
{

namespace GDH2
{

/* Opcodes from 0x10 to 0x1f could be used for key excange */
enum Opcodes {
    CMD_GDH2_NODE_VAL = 0x25,
    CMD_GDH2_LAST_VAL = 0x26,
};

struct network_t {
    uint16_t id, prev, next, first, last;
    uint16_t pos; // Hold a 1-based position (1: first node)

    network_t (const nodeset_t &nodes) {
        id = sKConfig.id(); 

        prev = next = first = last = id;
        pos = 1;

        for(nodeset_t::iterator i = nodes.begin(); i != nodes.end(); i++) {
            if(*i < first) first = *i;
            if(*i > last) last = *i;
            if((prev == id || (prev < id && *i > prev)) && *i < id) prev = *i;
            if((next == id || (next >= id && *i < next)) && *i > id) next = *i;
            if(*i < id) ++pos;
        }

        if(prev == id) prev = last;
        if(next == id) next = first;

        LOGP("Updated values (<:%04x ME:%04x >:%04x) (1:%04x N:%04x). Pos: %u", 
             prev, id, next, first, last, pos);
    }
    
    TAG_DEF("network_t")
};

class Behavior {
public:
    Behavior(PKTWrapper &pkt, const nodeset_t& nodes) : 
            _pkts(pkt), _nodes(nodes), _keylen(0), _completed(false) { 
        KEYBENCHMARK_START(gdh2_init);
        _inited = !gdh2_init(&_ctx, grp_id, nodes.size(), sKConfig.ctr_drbg());
        KEYBENCHMARK_STOP(gdh2_init);
        _tmpbuflen = GDH2_BUFSIZE * (nodes.size() + 1);
        _tmpbuf = new uint8_t[_tmpbuflen];
    }

    virtual ~Behavior() {
        if(_inited) 
            gdh2_free(&_ctx);
        memset(_key, 0, sizeof(_key));
        memset(_tmpbuf, 0, _tmpbuflen);
        delete[] _tmpbuf;
    }

    virtual bool init() = 0;
    virtual bool on_recv(const pkt_t &pkt) = 0;
    bool completed() const { return _completed; }

    const uint8_t *key() const { return _key; }
    size_t size() const { return _keylen; }

protected:
    PKTWrapper &_pkts;
    const nodeset_t &_nodes;

    size_t _keylen;
    uint8_t _key[GDH2_BUFSIZE];
    uint8_t *_tmpbuf;
    size_t _tmpbuflen;

    bool _completed;
    bool _inited;
    gdh2_context _ctx;

private:
    ecp_group_id grp_id = POLARSSL_ECP_DP_SECP256R1;
};

class FirstNodeBehavior : public Behavior {
public:
    FirstNodeBehavior(PKTWrapper &pkt, const nodeset_t& nodes, 
                      const network_t & net) : Behavior(pkt, nodes), 
                                               net(net) { } 

    bool init() { 
        if(!_inited)
            return false;

        size_t olen;

        LOG("Computing the first value.");
        KEYBENCHMARK_START(gdh2_make_firstval);
        if(gdh2_make_firstval(&_ctx, &olen, _tmpbuf, _tmpbuflen,
                            sKConfig.ctr_drbg())) {
            LOG("gdh2_make_firstval failed.");
            return false;
        }
        KEYBENCHMARK_STOP(gdh2_make_firstval);

        LOGP("Sending the value to %04x (len=%lu/%lu)", net.next, 
             olen, _tmpbuflen);
        _pkts.write(net.next, CMD_GDH2_NODE_VAL, _tmpbuf, olen);

        return true;
    }

    bool on_recv(const pkt_t& pkt) {
        if(!_inited)
            return false;

        if(pkt.src != net.last)
            return true;

        switch(pkt.cmd) {
            case CMD_GDH2_LAST_VAL: 
                KEYBENCHMARK_START(gdh2_compute_key);
                if(gdh2_compute_key(&_ctx, net.pos, &pkt.data[0], 
                                 pkt.data.size(), &_keylen, _key, 
                                 GDH2_BUFSIZE, sKConfig.ctr_drbg())) {
                    LOG("gdh2_compute_key failed.");
                    return false;
                } 
                KEYBENCHMARK_STOP(gdh2_compute_key);

                _completed = true;
                break;
            default:
                break;
        }

        return true; 
    }

private:
    const network_t net;
    
    TAG_DEF("GDH2FirstNodeBehavior")
};

class LastNodeBehavior : public Behavior {
public:
    LastNodeBehavior(PKTWrapper &pkt, const nodeset_t& nodes, 
                     const network_t & net) : Behavior(pkt, nodes), 
                                              net(net) { } 

    bool init() { 
        if(!_inited)
            return false;

        return true;
    }

    bool on_recv(const pkt_t& pkt) {
        if(!_inited)
            return false;

        if(pkt.src != net.prev)
            return true;

        switch(pkt.cmd) {
            case CMD_GDH2_NODE_VAL:
                size_t olen;
                KEYBENCHMARK_START(gdh2_make_val);
                if(gdh2_make_val(&_ctx, net.pos, &pkt.data[0], pkt.data.size(), 
                                 &olen, _tmpbuf, _tmpbuflen, 
                                 sKConfig.ctr_drbg())) {
                    LOG("gdh2_make_val failed.");
                    return false;
                } 
                KEYBENCHMARK_STOP(gdh2_make_val);

                LOGP("Broadcasting value (Size: %lu)", _tmpbuflen);
                _pkts.write(CMD_GDH2_LAST_VAL, _tmpbuf, olen);

                KEYBENCHMARK_START(gdh2_export_key);
                if(gdh2_export_key(&_ctx, &_keylen, _key, GDH2_BUFSIZE)) {
                    LOG("gdh2_export_key failed.");
                    return false;
                }
                KEYBENCHMARK_STOP(gdh2_export_key);

                _completed = true;
                break;
            default:
                break;
        }

        return true; 
    }

private:
    const network_t net;
    
    TAG_DEF("GDH2LastNodeBehavior")
};

class SimpleNodeBehavior : public Behavior {
public:
    SimpleNodeBehavior(PKTWrapper &pkt, const nodeset_t& nodes, 
                       const network_t & net) : Behavior(pkt, nodes), 
                                                net(net) { } 

    bool init() { 
        if(!_inited)
            return false;

        return true;
    }

    bool on_recv(const pkt_t& pkt) {
        size_t olen;

        if(!_inited)
            return false;

        switch(pkt.cmd) {
            case CMD_GDH2_NODE_VAL:
                if(pkt.src != net.prev)
                    return true;

                LOGP("Got a packet from the prev node (%04x). Working on it...",
                     net.prev);
                KEYBENCHMARK_START(gdh2_make_val);
                if(gdh2_make_val(&_ctx, net.pos, &pkt.data[0], pkt.data.size(), 
                                 &olen, _tmpbuf, _tmpbuflen, 
                                 sKConfig.ctr_drbg())) {
                    LOG("gdh2_make_val failed.");
                    return false;
                } 
                KEYBENCHMARK_STOP(gdh2_make_val);

                LOGP("Sending packet to the next node (%04x)", net.next);
                _pkts.write(net.next, CMD_GDH2_NODE_VAL, _tmpbuf, olen);
                break;
            case CMD_GDH2_LAST_VAL: 
                if(pkt.src != net.last)
                    return true;

                LOGP("Got the broadcast from the last node (%04x). Size: %04x", 
                     net.last, pkt.data.size());
                KEYBENCHMARK_START(gdh2_compute_key);
                if(gdh2_compute_key(&_ctx, net.pos, &pkt.data[0], 
                                    pkt.data.size(), &_keylen, _key, 
                                    GDH2_BUFSIZE, sKConfig.ctr_drbg())) {
                    LOG("gdh2_compute_key failed.");
                    return false;
                }
                KEYBENCHMARK_STOP(gdh2_compute_key);
                _completed = true;
                break;
            default:
                break;
        }

        return true; 
    }

private:
    const network_t net;
    
    TAG_DEF("GDH2SimpleNodeBehavior")
};


} /* GDH2 */

} /* KNX */

#endif /* ifndef KNX_CRYPTO_GDH2_UTILS_H */
