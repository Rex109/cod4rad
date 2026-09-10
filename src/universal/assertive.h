/* Original: ..\src\universal\assertive.h */

#ifndef ASSERTIVE_H
#define ASSERTIVE_H

#define ASSERT_TYPE_ASSERT          0
#define ASSERT_TYPE_SANITY          1
#define ASSERT_TYPE_INTERNAL_ERROR  2

void AssertFailed( const char *file, int line, int type, const char *fmt, ... );  /* 0x00402180 */

/* g_assertsDisabled  0x00534794 */
extern int g_assertsDisabled;


#define Assert( exp ) \
    ( ( exp ) ? ( void )0 : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, "%s", #exp ) )

#define Assertx( exp, fmt, ... ) \
    ( ( exp ) ? ( void )0 : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, "%s\n\t" fmt, #exp, __VA_ARGS__ ) )

#define SanityCheck( exp ) \
    ( ( exp ) ? ( void )0 : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_SANITY, "%s", #exp ) )

#define SanityCheckx( exp, fmt, ... ) \
    ( ( exp ) ? ( void )0 : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_SANITY, "%s\n\t" fmt, #exp, __VA_ARGS__ ) )

#define AssertMsg( msg ) \
    ( g_assertsDisabled ? ( void )0 : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, msg ) )

#define SanityCheckMsg( msg ) \
    ( g_assertsDisabled ? ( void )0 : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_SANITY, msg ) )


#define AssertCmp( a, op, b ) \
    ( ( ( a ) op ( b ) ) ? ( void )0 \
      : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, #a " " #op " " #b "\n\t%i, %i", a, b ) )

#define SanityCheckCmp( a, op, b ) \
    ( ( ( a ) op ( b ) ) ? ( void )0 \
      : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_SANITY, #a " " #op " " #b "\n\t%i, %i", a, b ) )

#define AssertCmpFloat( a, op, b ) \
    ( ( ( a ) op ( b ) ) ? ( void )0 \
      : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, #a " " #op " " #b "\n\t%g, %g", a, b ) )

#define SanityCheckCmpFloat( a, op, b ) \
    ( ( ( a ) op ( b ) ) ? ( void )0 \
      : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_SANITY, #a " " #op " " #b "\n\t%g, %g", a, b ) )

#define AssertIn( i, count ) \
    ( ( ( i ) >= 0 && ( i ) < ( count ) ) ? ( void )0 \
      : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, \
                      #i " doesn't index " #count "\n\t%i not in [0, %i)", i, count ) )

#define SanityCheckIn( i, count ) \
    ( ( ( i ) >= 0 && ( i ) < ( count ) ) ? ( void )0 \
      : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_SANITY, \
                      #i " doesn't index " #count "\n\t%i not in [0, %i)", i, count ) )

#define AssertRange( v, lo, hi ) \
    ( ( ( v ) >= ( lo ) && ( v ) <= ( hi ) ) ? ( void )0 \
      : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, \
                      #v " not in [" #lo ", " #hi "]\n\t%i not in [%i, %i]", v, lo, hi ) )

#define SanityCheckRange( v, lo, hi ) \
    ( ( ( v ) >= ( lo ) && ( v ) <= ( hi ) ) ? ( void )0 \
      : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_SANITY, \
                      #v " not in [" #lo ", " #hi "]\n\t%i not in [%i, %i]", v, lo, hi ) )

#define AssertRangeFloat( v, lo, hi ) \
    ( ( ( v ) >= ( lo ) && ( v ) <= ( hi ) ) ? ( void )0 \
      : AssertFailed( __FILE__, __LINE__, ASSERT_TYPE_ASSERT, \
                      #v " not in [" #lo ", " #hi "]\n\t%g not in [%g, %g]", v, lo, hi ) )

template< class Type >
inline Type checked_cast( int i )
{
    AssertCmp( i, ==, static_cast< Type >( i ) );
    return static_cast< Type >( i );
}


typedef void ( *AssertHandler_t )( const char *text );

void Assertive_SetHandler( AssertHandler_t handler );          /* 0x00402170 */
void Assertive_Init( void );                                   /* 0x00402160 */
void Assertive_OutOfMemory( const char *file, int line );      /* 0x00402500 */
void Assertive_ResetDisplay( void );                           /* 0x00401000 */

#endif
