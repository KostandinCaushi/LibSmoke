/** 
 * \file common.h
 *
 * \brief Common file
 */

#ifndef SBAN_COMMON_H
#define SBAN_COMMON_H

#include <polarssl/include/polarssl/ecp.h>
#include <polarssl/include/polarssl/bignum.h>
#include <polarssl/include/polarssl/ctr_drbg.h>

#define BAD_INPUT_DATA            -1
#define BUFFER_TOO_SHORT          -2
#define MAX_ROUND_NUMBER_EXCEEDED -3
#define ROUND_NOT_COMPLETED	      -4

#endif
