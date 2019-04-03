#include <sban/include/sban/util.h>

/* Taken from https://tls.mbed.org/discussions/generic/compute-inverse-of-ecp */
int ecp_opp(const ecp_group *grp, ecp_point *R, const ecp_point *P)
{
    int ret;

    /* Copy */
    if( R != P )
        MPI_CHK( ecp_copy( R, P ) );

    /* In-place opposite */
    if( mpi_cmp_int( &R->Y, 0 ) != 0 )
        MPI_CHK(mpi_sub_mpi( &R->Y, &grp->P, &R->Y ));

cleanup:
    return( ret );
}

/** ------------------------- STOLEN FROM ecp.c ---------------------------- */

#if defined(POLARSSL_ECP_DP_SECP192R1_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_SECP224R1_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_SECP256R1_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_SECP384R1_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_SECP521R1_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_BP256R1_ENABLED)   ||   \
    defined(POLARSSL_ECP_DP_BP384R1_ENABLED)   ||   \
    defined(POLARSSL_ECP_DP_BP512R1_ENABLED)   ||   \
    defined(POLARSSL_ECP_DP_SECP192K1_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_SECP224K1_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_SECP256K1_ENABLED)
#define POLARSSL_ECP_SHORT_WEIERSTRASS
#endif

#if defined(POLARSSL_ECP_DP_M221_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_M255_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_M383_ENABLED) ||   \
    defined(POLARSSL_ECP_DP_M511_ENABLED)
#define POLARSSL_ECP_MONTGOMERY
#endif

/*
 * Curve types: internal for now, might be exposed later
 */
typedef enum
{
    POLARSSL_ECP_TYPE_NONE = 0,
    POLARSSL_ECP_TYPE_SHORT_WEIERSTRASS,    /* y^2 = x^3 + a x + b      */
    POLARSSL_ECP_TYPE_MONTGOMERY,           /* y^2 = x^3 + a x^2 + x    */
} ecp_curve_type;

/*
 * Get the type of a curve
 */
static inline ecp_curve_type ecp_get_type( const ecp_group *grp )
{
    if( grp->G.X.p == NULL )
        return( POLARSSL_ECP_TYPE_NONE );

    if( grp->G.Y.p == NULL )
        return( POLARSSL_ECP_TYPE_MONTGOMERY );
    else
        return( POLARSSL_ECP_TYPE_SHORT_WEIERSTRASS );
}

int ecp_gen_privkey( ecp_group *grp, mpi *d,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng )
{
    int ret;
    size_t n_size = (grp->nbits + 7) / 8;

#if defined(POLARSSL_ECP_MONTGOMERY)
    if( ecp_get_type( grp ) == POLARSSL_ECP_TYPE_MONTGOMERY )
    {
        /* [M225] page 5 */
        size_t b;

        MPI_CHK( mpi_fill_random( d, n_size, f_rng, p_rng ) );

        /* Make sure the most significant bit is nbits */
        b = mpi_msb( d ) - 1; /* mpi_msb is one-based */
        if( b > grp->nbits )
            MPI_CHK( mpi_shift_r( d, b - grp->nbits ) );
        else
            MPI_CHK( mpi_set_bit( d, grp->nbits, 1 ) );

        /* Make sure the last three bits are unset */
        MPI_CHK( mpi_set_bit( d, 0, 0 ) );
        MPI_CHK( mpi_set_bit( d, 1, 0 ) );
        MPI_CHK( mpi_set_bit( d, 2, 0 ) );
    }
    else
#endif /* POLARSSL_ECP_MONTGOMERY */
#if defined(POLARSSL_ECP_SHORT_WEIERSTRASS)
    if( ecp_get_type( grp ) == POLARSSL_ECP_TYPE_SHORT_WEIERSTRASS )
    {
        /* SEC1 3.2.1: Generate d such that 1 <= n < N */
        int count = 0;
        unsigned char rnd[POLARSSL_ECP_MAX_BYTES];

        /*
         * Match the procedure given in RFC 6979 (deterministic ECDSA):
         * - use the same byte ordering;
         * - keep the leftmost nbits bits of the generated octet string;
         * - try until result is in the desired range.
         * This also avoids any biais, which is especially important for ECDSA.
         */
        do
        {
            MPI_CHK( f_rng( p_rng, rnd, n_size ) );
            MPI_CHK( mpi_read_binary( d, rnd, n_size ) );
            MPI_CHK( mpi_shift_r( d, 8 * n_size - grp->nbits ) );

            /*
             * Each try has at worst a probability 1/2 of failing (the msb has
             * a probability 1/2 of being 0, and then the result will be < N),
             * so after 30 tries failure probability is a most 2**(-30).
             *
             * For most curves, 1 try is enough with overwhelming probability,
             * since N starts with a lot of 1s in binary, but some curves
             * such as secp224k1 are actually very close to the worst case.
             */
            if( ++count > 30 )
                return( POLARSSL_ERR_ECP_RANDOM_FAILED );
        }
        while( mpi_cmp_int( d, 1 ) < 0 ||
               mpi_cmp_mpi( d, &grp->N ) >= 0 );
    }
    else
#endif /* POLARSSL_ECP_SHORT_WEIERSTRASS */
        return( POLARSSL_ERR_ECP_BAD_INPUT_DATA );

cleanup:
    return( ret );
}

