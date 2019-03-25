#ifndef KNX_CONFIG_H
#define KNX_CONFIG_H

#include <knx/common.h>

namespace KNX
{

class DeviceConfig {
public:
    static DeviceConfig& _instance() { static DeviceConfig i; return i; }

    uint16_t id() const { return _id; }
    entropy_context &entropy() { return _entropy; }
    ctr_drbg_context *ctr_drbg() { return &_ctr_drbg; }
    bool ctr_drbg_available() { return _random_inited; }
    void random(uint8_t *buffer, size_t len) {
        ctr_drbg_random(&_ctr_drbg, buffer, len);
    }

private:
    entropy_context _entropy;
    ctr_drbg_context _ctr_drbg;
    bool _random_inited;
    uint16_t _id;

    DeviceConfig() : _random_inited(false) {
        static const char *seed = "HopingToGetAGoodGrade.(C)2016 Al";
        entropy_init(&_entropy);

        if(!ctr_drbg_init(&_ctr_drbg, entropy_func, &_entropy, 
                          (const uint8_t *)seed, strlen(seed)))
            if(!ctr_drbg_random(&_ctr_drbg, (uint8_t *)&_id, sizeof(_id)))
                _random_inited = true;

        if(!_random_inited)
            LOG("Random Generator is not available.");

    }

    ~DeviceConfig() {
        entropy_free(&_entropy);
    }

    void operator=(const DeviceConfig&) = delete;
    DeviceConfig(const DeviceConfig&) = delete;

    TAG_DEF("DeviceConfig")
};

#define sKConfig DeviceConfig::_instance()

} /* KNX */

#endif /* ifndef KNX_CONFIG_H */
