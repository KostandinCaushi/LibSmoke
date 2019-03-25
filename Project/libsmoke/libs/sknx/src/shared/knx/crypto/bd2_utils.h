#ifndef KNX_CRYPTO_BD2_UTILS_H
#define KNX_CRYPTO_BD2_UTILS_H

#include <knx/common.h>

extern "C" {
#include <sban/bd2.h>
}

#define BD2_BUFSIZE 66
namespace KNX
{

namespace BD2
{

/* Opcodes from 0x20 to 0x2f are reserved for key exchange */
enum Opcodes {
    CMD_BD2_NODE_Zi     = 0x22,
    CMD_BD2_CHAIR_Z0    = 0x23,
    CMD_BD2_CHAIR_Xi    = 0x24,
};

class Behavior {
public:
    Behavior(PKTWrapper &pkt) : _pkts(pkt), _keylen(0),
                                _completed(false) { 
        
        _inited = !bd2_init(&_ctx, grp_id);
    }

    virtual ~Behavior() {
        if(_inited) 
            bd2_free(&_ctx);
        memset(_key, 0, sizeof(_key));
    }

    virtual bool init() = 0;
    virtual bool on_recv(const pkt_t &pkt) = 0;
    bool completed() const { return _completed; }

    const uint8_t *key() const { return _key; }
    size_t size() const { return _keylen; }

protected:
    PKTWrapper &_pkts;

    size_t _keylen;
    uint8_t _key[BD2_BUFSIZE];

    bool _completed;
    bool _inited;
    bd2_context _ctx;

private:
    ecp_group_id grp_id = POLARSSL_ECP_DP_SECP256R1;
};

class ChairBehavior : public Behavior {
public:
    ChairBehavior(PKTWrapper &pkt, const nodeset_t & nodes) : Behavior(pkt),
                                                              _nodes(nodes) { } 

    bool init() { 
        if(!_inited)
            return false;

        size_t olen;
        
        KEYBENCHMARK_START(bd2_make_public);
        if(bd2_make_public(&_ctx, &olen, _pub, sizeof(_pub),
                           sKConfig.ctr_drbg())) {
            LOG("bd2_make_public failed.");
            return false;
        }
        KEYBENCHMARK_STOP(bd2_make_public);

        LOGP("bd2_make_public just generated a value (olen: %lu)", olen);
        _pkts.write(CMD_BD2_CHAIR_Z0, _pub, sizeof(_pub));

        KEYBENCHMARK_START(bd2_make_key);
        if(bd2_make_key(&_ctx, sKConfig.ctr_drbg())) { 
            LOG("bd2_make_key failed.");
            return false;
        }
        KEYBENCHMARK_STOP(bd2_make_key);

        KEYBENCHMARK_START(bd2_export_key);
        if(bd2_export_key(&_ctx, &_keylen, _key, sizeof(_key))) {
            LOG("bd2_export_key failed.");
            return false;
        }
        KEYBENCHMARK_STOP(bd2_export_key);

        return true;
    }

    bool on_recv(const pkt_t& pkt) {
        if(!_inited)
            return false;

        if(pkt.data.size() != BD2_BUFSIZE)
            return true;

        if(pkt.dest != sKConfig.id()) {
            LOGP("Discarding an unexpected packet from %04x to %04x", 
                 pkt.src, pkt.dest);
            return true;
        }

        switch(pkt.cmd) {
            case CMD_BD2_NODE_Zi: 
                _bd2_keys_t k;
                k.id = pkt.src;
                memcpy(k.key, &pkt.data[0], BD2_BUFSIZE);
                _nodes_queue.push(k);
                break;
        }

        if(_nodes_queue.size() == _nodes.size() - 1) {
            while(!_nodes_queue.empty()) {
                uint8_t temp[BD2_BUFSIZE];
                const _bd2_keys_t &k = _nodes_queue.top();
                if(bd2_read_public(&_ctx, &k.key[0], BD2_BUFSIZE)) {
                    LOG("bd2_read_public failed.");
                    return false;
                }

                size_t olen;
                
                KEYBENCHMARK_START(bd2_make_val);
                if(bd2_make_val(&_ctx, &olen, temp, sizeof(temp), 
                             sKConfig.ctr_drbg())) {
                    LOG("bd2_make_val failed.");
                    return false;
                }
                KEYBENCHMARK_STOP(bd2_make_val);

                // Send X_i to the i_th client
                _pkts.write(k.id, CMD_BD2_CHAIR_Xi, temp, sizeof(temp));
                _nodes_queue.pop();
            }
            _completed = true;
        } 

        return true; 
    }

private:
    const nodeset_t &_nodes;
    uint8_t _pub[BD2_BUFSIZE];
    
    typedef struct _bd2_keys_t {
        uint16_t id;
        uint8_t key[BD2_BUFSIZE];

        /** Decreasing order */
        bool operator<(const _bd2_keys_t &rhs) const { 
            return (id > rhs.id);
        }
    } _bd2_keys_t;

    std::priority_queue<_bd2_keys_t> _nodes_queue;
    TAG_DEF("BD2ChairBehavior")
};

class NodeBehavior : public Behavior {
public:
    NodeBehavior(PKTWrapper &pkt, uint16_t chair) : Behavior(pkt), 
                                                    _chair(chair) {}
    
    bool init() {
        if(!_inited)
            return false;

        return true;
    }

    bool on_recv(const pkt_t& pkt) {
        if(!_inited)
            return false;

        if(pkt.data.size() != BD2_BUFSIZE)
            return true;

        if(pkt.src != _chair)
            return true;

        switch(pkt.cmd) {
            case CMD_BD2_CHAIR_Z0:
                
                KEYBENCHMARK_START(bd2_read_public);
                if(bd2_read_public(&_ctx, &pkt.data[0], pkt.data.size()))
                    return false;
                KEYBENCHMARK_STOP(bd2_read_public);

                size_t olen;
                uint8_t tmp[BD2_BUFSIZE];
                
                KEYBENCHMARK_START(bd2_make_public);
                if(bd2_make_public(&_ctx, &olen, tmp, sizeof(tmp),
                                   sKConfig.ctr_drbg())) {
                    LOG("bd2_make_public failed.");
                    return false;
                }
                KEYBENCHMARK_STOP(bd2_make_public);

                // Broadcast Z_i
                _pkts.write(_chair, CMD_BD2_NODE_Zi, tmp, sizeof(tmp));
                break; 
            case CMD_BD2_CHAIR_Xi:
                KEYBENCHMARK_START(bd2_import_val);
                if(bd2_import_val(&_ctx, &pkt.data[0], pkt.data.size())) {
                    LOG("bd2_import_val failed.");
                    return false;
                }
                KEYBENCHMARK_STOP(bd2_import_val);

                KEYBENCHMARK_START(bd2_calc_key);
                if(bd2_calc_key(&_ctx, sKConfig.ctr_drbg())) {
                    LOG("bd2_calc_key failed.");
                    return false;
                }
                KEYBENCHMARK_STOP(bd2_calc_key);

                KEYBENCHMARK_START(bd2_export_key);
                if(bd2_export_key(&_ctx, &_keylen, _key, sizeof(_key))) {
                    LOG("bd2_export_key failed.");
                    return false;
                }
                KEYBENCHMARK_STOP(bd2_export_key);

                _completed = true;
                break;
        }

        return true;
    }

private:
    const uint16_t _chair;
    TAG_DEF("BD2NodeBehavior")
};

} /* BD2 */

} /* KNX */

#endif /* ifndef KNX_CRYPTO_BD2_UTILS_H */
