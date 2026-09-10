/* Original: ..\src\physics\ode\src\matrix.cpp */

#ifndef ODE_MATRIX_H
#define ODE_MATRIX_H

#include "../../../universal/q_shared.h"
#include "../../../universal/assertive.h"


typedef float dReal;

typedef dReal dVector3[4];
typedef dReal dVector4[4];
typedef dReal dMatrix3[4*3];

#define dAASSERT( a ) \
    ( ( a ) ? ( void )0 : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, "%s", #a ) )
#define dIASSERT( a ) \
    ( ( a ) ? ( void )0 : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, "%s", #a ) )
#define dUASSERT( a, msg ) \
    ( ( a ) ? ( void )0 : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, "%s\n\t%s", #a, msg ) )

#ifndef M_PI
#define M_PI        3.1415926535897932384626433832795029
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2   0.7071067811865475244008443621048490
#endif

#define dPAD( a )   ( ( (a) > 1 ) ? ( ( ( (a) - 1 ) | 3 ) + 1 ) : (a) )

#define REAL( x )   ( x )

__forceinline dReal dRecip( dReal x ) { return REAL( 1.0 ) / x; }

#define dSqrt( x )   sqrtf( x )

void *dAllocaNotSupported( int size );
#define ALLOCA( x )   dAllocaNotSupported( x )

#define dCROSSMAT(a,b,skip,plus,minus) \
do { \
  (a)[1] = minus (b)[2]; \
  (a)[2] = plus (b)[1]; \
  (a)[(skip)+0] = plus (b)[2]; \
  (a)[(skip)+2] = minus (b)[0]; \
  (a)[2*(skip)+0] = minus (b)[1]; \
  (a)[2*(skip)+1] = plus (b)[0]; \
} while(0)

dReal dDOT   ( const dReal *a, const dReal *b );                    /* 0x0048f830 */
dReal dDOT14 ( const dReal *a, const dReal *b );                    /* 0x0048f700 */

void dMultiply0_331( dReal *A, const dReal *B, const dReal *C );    /* 0x0048f870 */
void dMultiply0_333( dReal *A, const dReal *B, const dReal *C );    /* 0x0048f610 */
void dMultiply2_333( dReal *A, const dReal *B, const dReal *C );    /* 0x0048f740 */

void  dSetZero  ( dReal *a, int n );                                /* 0x0048f8c0 */
void  dSetValue ( dReal *a, int n, dReal value );                   /* 0x0048f910 */
dReal dDot      ( const dReal *a, const dReal *b, int n );          /* 0x0048d6c0 */

void dMultiply0 ( dReal *A, const dReal *B, const dReal *C, int p, int q, int r );
                                                                    /* 0x0048f960 */
void dMultiply1 ( dReal *A, const dReal *B, const dReal *C, int p, int q, int r );
                                                                    /* 0x0048fac0 */
void dMultiply2 ( dReal *A, const dReal *B, const dReal *C, int p, int q, int r );
                                                                    /* 0x0048fbf0 */

int  dFactorCholesky3     ( dReal *A );                             /* 0x0048fd50 */
void dSolveCholesky3      ( const dReal *L, dReal *b );             /* 0x0048ff20 */
int  dInvertPDMatrix3     ( const dReal *A, dReal *Ainv );          /* 0x004900f0 */
int  dIsPositiveDefinite3 ( const dReal *A );                       /* 0x00490210 */

void dSolveL1  ( const dReal *L, dReal *B, int n, int lskip1 );     /* 0x0048d780 */
void dSolveL1T ( const dReal *L, dReal *B, int n, int lskip1 );     /* 0x0048e0a0 */

void dVectorScale ( dReal *a, const dReal *d, int n );              /* 0x00490270 */
void dSolveLDLT   ( const dReal *L, const dReal *d, dReal *b, int n, int nskip );
                                                                    /* 0x004902f0 */
void dLDLTAddTL   ( dReal *L, dReal *d, const dReal *a, int n, int nskip );
                                                                    /* 0x00490380 */
void dLDLTRemove  ( dReal **A, const int *p, dReal *L, dReal *d,
                    int n1, int n2, int r, int nskip );             /* 0x00490710 */
void dRemoveRowCol( dReal *A, int n, int nskip, int r );            /* 0x00490a50 */

#endif
