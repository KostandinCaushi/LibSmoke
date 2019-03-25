#ifndef POLARSSL_MIOSIX_POLL_H
#define POLARSSL_MIOSIX_POLL_H

#if defined(_MIOSIX)
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief           miosix-based entropy poll callback
 */
extern int miosix_poll(void *data, 
                       unsigned char *output, size_t len, size_t *olen);

#ifdef __cplusplus
}
#endif

#endif /* _MIOSIX */

#endif /* POLARSSL_MIOSIX_POLL_H */
