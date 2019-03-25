/** 
 * \file util.h
 *
 * \brief Util functions
 */

#ifndef SBAN_UTIL_H
#define SBAN_UTIL_H

#include "common.h"

int ecp_opp(const ecp_group *grp, ecp_point *R, const ecp_point *P);

/** Mostly copied from ecp_gen_keypair */
int ecp_gen_privkey( ecp_group *grp, mpi *d,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng );

#endif /* SBAN_UTIL_H */
