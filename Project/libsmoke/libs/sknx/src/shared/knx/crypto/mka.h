#ifndef KNX_CRYPTO_MKA_H
#define KNX_CRYPTO_MKA_H

#include "crypto.h"
#include <string.h>

extern "C" {
#include "../../../../libs/sban/include/sban/mka.h"
}

#define MKA_BUFSIZE 66

namespace KNX
{

enum MKAOpcodes {
    CMD_MKA_PACKET = 0x27,
};

class MKAKeyExchange : public KeyExchange {

public:
    MKAKeyExchange(PKTWrapper& pkt) : KeyExchange(), _pkts(pkt) { }

    ~MKAKeyExchange() {
        shutdown();
    }

    bool init(const nodeset_t &nodes) {
        if(_status != KEYEXCHANGE_OFFLINE)
            return false;

        if(nodes.size() > 255 || nodes.size() < 2) // I'm using uint8_t!
            return false;

        _status = KEYEXCHANGE_RUNNING;
        _nodes = nodes;

        if(!sKConfig.ctr_drbg_available()) {
            LOG("Random generator is not available.");
            _status = KEYEXCHANGE_ERROR;
            return false;
        }

        KEYBENCHMARK_START(mka_init);
        if(mka_init(&_ctx, _grp_id)) {
            LOG("mka_init failed.");
            _status = KEYEXCHANGE_ERROR;
            return false;
        }
        KEYBENCHMARK_STOP(mka_init);

        KEYBENCHMARK_START(mka_set_part);
        if(mka_set_part(&_ctx, _nodes.size(), sKConfig.ctr_drbg())) {
            LOG("mka_set_part failed.");
            mka_free(&_ctx);
            _status = KEYEXCHANGE_ERROR;
            return false;
        }
        KEYBENCHMARK_STOP(mka_set_part);

        _remote_keys.resize(MKA_BUFSIZE * (_nodes.size() - 1));
        _remote_users.clear();
        _current_step = 0;

        if(!_crypto_broadcast())
            return false;

        return true;
    }

    void shutdown() {
        if(_status != KEYEXCHANGE_OFFLINE && _status != KEYEXCHANGE_ERROR)
            mka_free(&_ctx);
        _status = KEYEXCHANGE_OFFLINE;
        memset(_keyBuffer, 0, sizeof(_keyBuffer));
    }

    bool update() {
        if(_status != KEYEXCHANGE_RUNNING) 
            return false;

        while(_pkts.count() > 0 && _current_step < _nodes.size() - 1) {
            pkt_t packet;

            _pkts.read(packet);

            if(packet.cmd != CMD_MKA_PACKET) 
                continue;

            if(_nodes.find(packet.src) == _nodes.end())
                continue;

            if(!_crypto_update(packet))
                return false;
        }

        if(_current_step == _nodes.size() - 1) {
            KEYBENCHMARK_START(mka_compute_key);
            if(mka_compute_key(&_ctx, &_keylen, _keyBuffer, 
                               sizeof(_keyBuffer), sKConfig.ctr_drbg())) {
                LOG("mka_compute_key failed.");
                _status = KEYEXCHANGE_ERROR;
                mka_free(&_ctx);
                return false;
            }
            KEYBENCHMARK_STOP(mka_compute_key);

            _status = KEYEXCHANGE_COMPLETED;
        }

        return true;
    }

    void operator=(const KNX::MKAKeyExchange &data) {
        _keylen = data.size();
        memcpy(_keyBuffer, data.key(), data.size());
    }

    const uint8_t * key() const { return _keyBuffer; }
    size_t size() const { return _keylen; }

private:
    PKTWrapper& _pkts;
    uint8_t _current_step;
    uint8_t _keyBuffer[MKA_BUFSIZE];
    size_t _keylen;

#pragma pack(1)
    typedef struct {
        uint8_t step_id;
        uint8_t key[MKA_BUFSIZE]; 
    } mka_step_t;
#pragma pack()

    /** Temporary storage for keys used in the next step */
    typedef std::pair<uint16_t,mka_step_t> _next_step_t;
    std::queue<_next_step_t> _next_step_values;

    std::vector<uint8_t> _remote_keys;
    std::set<uint16_t> _remote_users;

    static const ecp_group_id _grp_id = POLARSSL_ECP_DP_SECP256R1;
    mka_context _ctx;

    bool _crypto_broadcast() {
        mka_step_t cur_step;
        size_t key_len;

        cur_step.step_id = _current_step;
        KEYBENCHMARK_START(mka_make_broadval);
        if(mka_make_broadval(&_ctx, &key_len, cur_step.key, 
                             sizeof(cur_step.key), sKConfig.ctr_drbg())) {
            LOG("mka_make_broadval failed.");
            mka_free(&_ctx);
            _status = KEYEXCHANGE_ERROR;
            return false;
        }
        KEYBENCHMARK_STOP(mka_make_broadval);

        LOGP("Step %u/%lu....", _current_step + 1, _nodes.size() - 1);
        _pkts.write(CMD_MKA_PACKET, (const uint8_t *)&cur_step, 
                    sizeof(cur_step));
        return true;
    }

    bool _crypto_update(const pkt_t& packet) {
        if(packet.data.size() != sizeof(mka_step_t))
            return true;

        const mka_step_t *mka = 
            reinterpret_cast<const mka_step_t *>(&packet.data[0]);

        if(mka->step_id == _current_step) {
            /* Not inserted yet */
            if(_remote_users.find(packet.src) == _remote_users.end()) {
                size_t pos = _remote_users.size();
                memcpy(&_remote_keys[pos * MKA_BUFSIZE], 
                       &mka->key, MKA_BUFSIZE);
                _remote_users.insert(packet.src);
            }
        }
        else if(mka->step_id == _current_step + 1)
            _next_step_values.push(_next_step_t(packet.src, *mka));

        /* Just received all keys */
        if(_remote_users.size() == _nodes.size() - 1) {
            KEYBENCHMARK_START(mka_acc_broadval);
            if(mka_acc_broadval(&_ctx, &_remote_keys[0], _remote_keys.size(), 
                                sKConfig.ctr_drbg())){
                LOG("mka_acc_broadval failed.\n");
                _status = KEYEXCHANGE_ERROR;
                mka_free(&_ctx);
                return false;
            }
            KEYBENCHMARK_STOP(mka_acc_broadval);
                
            _remote_users.clear();
            _current_step++;

            if(_current_step == _nodes.size() - 1)
                return true;

            if(!_crypto_broadcast())
                return false;

            while(!_next_step_values.empty()) {
                _next_step_t &mks = _next_step_values.front();
                _next_step_values.pop();

                if(mks.second.step_id != _current_step)
                    continue;

                if(_remote_users.find(mks.first) == _remote_users.end()) {
                    size_t pos = _remote_users.size();
                    memcpy(&_remote_keys[pos * MKA_BUFSIZE], 
                        &mka->key, MKA_BUFSIZE);
                    _remote_users.insert(mks.first);
                }
            }
        }

        return true;
    }
    TAG_DEF("MKAKeyExchange")
};

} /* KNX */

#endif /* ifndef KNX_CRYPTO_MKA_H */
