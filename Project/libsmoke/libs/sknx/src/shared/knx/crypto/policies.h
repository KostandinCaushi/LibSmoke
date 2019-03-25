#ifndef KNX_CRYPTO_POLICIES_H
#define KNX_CRYPTO_POLICIES_H

#include <knx/common.h>
#include "crypto.h"

namespace KNX
{

namespace Policies
{

class LowestIdNodeChooser {
protected:
    uint16_t get_node(const nodeset_t& nodes) {
        uint16_t n = 0;
        bool chosen = false;
        for(nodeset_t::iterator i = nodes.begin(); i != nodes.end(); i++) {
            if(!chosen || n > *i) {
                n = *i;
                chosen = true;
            }
        }
        return n;
    }
};

};

}; /* KNX */

#endif /* ifndef KNX_CRYPTO_POLICIES_H */
