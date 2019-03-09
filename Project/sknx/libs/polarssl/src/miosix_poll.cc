#include "polarssl/miosix_poll.h"

#ifdef _MIOSIX 

#include <miosix.h>
#include <drivers/stm32_hardware_rng.h>

int miosix_poll(void *data, 
                unsigned char *output, size_t len, size_t *olen) {
    miosix::HardwareRng::instance().get(output, len);
    *olen = len;
    return 0;	
}

#endif
