#ifndef KNX_CRYPTO_BD1_H
#define KNX_CRYPTO_BD1_H

#include "crypto.h"

extern "C" {
#include <sban/bd1.h>
}

#define BD1_BUFSIZE 66

namespace KNX
{

/* Opcodes from 0x20 to 0x2f are reserved for key exchange */
enum BD1Opcodes {
    CMD_BD1_Zi  = 0x20,
    CMD_BD1_Xi  = 0x21,
};

class BD1KeyExchange : public KeyExchange {

public:
    BD1KeyExchange(PKTWrapper& pkt) : KeyExchange(), _pkts(pkt) { }

    ~BD1KeyExchange() {
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

        KEYBENCHMARK_START(bd1_init);
        if(bd1_init(&_ctx, _grp_id)) {
            _status = KEYEXCHANGE_ERROR;
            return false;
        }
        KEYBENCHMARK_STOP(bd1_init);

        _get_prev_next_nodes();
        _recvZcnt = 0;

        size_t olen;
        uint8_t mykey[BD1_BUFSIZE];
        KEYBENCHMARK_START(bd1_gen_point);
        if(bd1_gen_point(&_ctx, &olen, mykey, BD1_BUFSIZE,
                         sKConfig.ctr_drbg())) {
            bd1_free(&_ctx);
            _status = KEYEXCHANGE_ERROR;
            return false;
        }
        KEYBENCHMARK_STOP(bd1_gen_point);

        LOG("Broadcasting my z_i...");
        _pkts.write(CMD_BD1_Zi, (const uint8_t *)&mykey, BD1_BUFSIZE);

        return true;
    }

    void shutdown() {
        if(_status != KEYEXCHANGE_OFFLINE && _status != KEYEXCHANGE_ERROR)
            bd1_free(&_ctx);
        _status = KEYEXCHANGE_OFFLINE;
    }

    bool update() {
        if(_status != KEYEXCHANGE_RUNNING) 
            return false;

        while(_pkts.count() > 0 && _X_keys.size() < _nodes.size()) {
            pkt_t packet;

            _pkts.read(packet);

            if(packet.data.size() != BD1_BUFSIZE)
                continue;

            switch(packet.cmd) {
                case CMD_BD1_Zi:
                    if(packet.src == _prev) {
                        memcpy(_Zi, &packet.data[0], BD1_BUFSIZE);
                        _recvZcnt |= 1;
                    }
                    if(packet.src == _next) {
                        memcpy(_Zi + BD1_BUFSIZE, &packet.data[0], BD1_BUFSIZE);
                        _recvZcnt |= 2;
                    }
                    break;
                case CMD_BD1_Xi:
                    _bd1_keys_t mykey;
                    mykey.id = packet.src;
                    memcpy(mykey.key, &packet.data[0], BD1_BUFSIZE);
                    _X_keys.push(mykey);
                    break;
            }
        }

        if(_recvZcnt == 3) { // 3 == 1 | 2
            _bd1_keys_t mykey;
            size_t olen;

            KEYBENCHMARK_START(bd1_gen_value);
            if(bd1_gen_value(&_ctx, _Zi, 2 * BD1_BUFSIZE, &olen, mykey.key, 
                             sizeof(mykey.key), sKConfig.ctr_drbg())) {
                LOG("bd1_gen_value failed.");
                bd1_free(&_ctx);
                _status = KEYEXCHANGE_ERROR;
                return false;
            }
            KEYBENCHMARK_STOP(bd1_gen_value);
                            
            LOG("Broadcasting my X_i...");

            _pkts.write(CMD_BD1_Xi, mykey.key, sizeof(mykey.key));
            mykey.id = sKConfig.id();
            _X_keys.push(mykey);
            _recvZcnt = 4; // A simple way to disable this if block 
        }

        if(_X_keys.size() == _nodes.size()) {
            uint8_t *tempbuf = new uint8_t[BD1_BUFSIZE * _nodes.size()];
            std::queue<_bd1_keys_t> q; 
        
            size_t i = 0;
            while(!_X_keys.empty() && i < _nodes.size()) {
                // TODO improve this code block!
                const _bd1_keys_t &k = _X_keys.top(); 
                if(k.id < sKConfig.id())
                    q.push(k);
                else if(k.id > sKConfig.id())
                    memcpy(&tempbuf[i++ * BD1_BUFSIZE], k.key, BD1_BUFSIZE);
                _X_keys.pop();
            }

            while(!q.empty()) {
                const _bd1_keys_t &k = q.front();
                memcpy(&tempbuf[i++ * BD1_BUFSIZE], k.key, BD1_BUFSIZE);
                q.pop();
            }

            KEYBENCHMARK_START(bd1_compute_key);
            if(bd1_compute_key(&_ctx, tempbuf, BD1_BUFSIZE * _nodes.size() - 1, 
                               _nodes.size(), &_keylen, _keyBuffer, 
                               sizeof(_keyBuffer), sKConfig.ctr_drbg())) {
                LOG("bd1_compute_key failed.");
                _status = KEYEXCHANGE_ERROR;
                bd1_free(&_ctx);
                delete[] tempbuf;
                return false;
            }
            KEYBENCHMARK_STOP(bd1_compute_key);

            _status = KEYEXCHANGE_COMPLETED;
            delete[] tempbuf;
        }


        return true;
    }

    const uint8_t * key() const { return _keyBuffer; }
    size_t size() const { return _keylen; }

private:
    PKTWrapper& _pkts;
    uint8_t _keyBuffer[BD1_BUFSIZE];
    size_t _keylen;

    typedef struct _bd1_keys_t {
        uint16_t id;
        uint8_t key[BD1_BUFSIZE];

        /** Decreasing order */
        bool operator<(const _bd1_keys_t &rhs) const { 
            return (id > rhs.id);
        }
    } _bd1_keys_t;

    std::priority_queue<_bd1_keys_t> _X_keys;

    static const ecp_group_id _grp_id = POLARSSL_ECP_DP_SECP256R1;
    bd1_context _ctx;
    uint16_t _prev, _next;

    // Will contain Z_{i-1} and Z_{i+1}
    uint8_t _Zi[BD1_BUFSIZE * 2], _recvZcnt;

    void _get_prev_next_nodes() {
        uint16_t id = sKConfig.id(); 
        uint16_t last = id, first = id;

        _prev = _next = id;

        for(nodeset_t::iterator i = _nodes.begin(); i != _nodes.end(); i++) {
            if(*i < first) first = *i;
            if(*i > last) last = *i;
            if((_prev == id || (_prev < id && *i > _prev)) && *i < id) 
                _prev = *i;
            if((_next == id || (_next >= id && *i < _next)) && *i > id) 
                _next = *i;
        }

        if(_prev == id) _prev = last;
        if(_next == id) _next = first;

        LOGP("My id: %04x. Prev: %04x. Next: %04x", id, _prev, _next);
    }

    TAG_DEF("BD1KeyExchange")
};

} /* KNX */

#endif /* ifndef KNX_CRYPTO_BD1_H */
