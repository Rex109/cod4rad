/* Original: ..\src\universal\com_math.cpp */

#include "q_shared.h"
#include "com_math.h"
#include "com_vector.h"


struct cplane_s
{
    vec3_t normal;      /* +0x00 */
    float  dist;        /* +0x0c */
    byte   type;        /* +0x10 */
    byte   signbits;    /* +0x11 */
    byte   pad[2];      /* +0x12 */
};

extern void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );

#include "../xanim/xanim_public.h"


const vec3_t vec3_origin = { 0.0f, 0.0f, 0.0f };            /* 0x004f1324 */
const vec4_t vec4_origin = { 0.0f, 0.0f, 0.0f, 0.0f };      /* 0x004f1330 */

static const float g_identityMatrix44[4][4] =               /* 0x0050abd8 */
{
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

#define NUMVERTEXNORMALS    162

static vec3_t bytedirs[NUMVERTEXNORMALS] =                  /* 0x005296b0 */
{
    { -0.525731f, 0.0f, 0.850651f }, { -0.442863f, 0.238856f, 0.864188f },
    { -0.295242f, 0.0f, 0.955423f }, { -0.309017f, 0.5f, 0.809017f },
    { -0.16246f, 0.262866f, 0.951056f }, { 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.850651f, 0.525731f }, { -0.147621f, 0.716567f, 0.681718f },
    { 0.147621f, 0.716567f, 0.681718f }, { 0.0f, 0.525731f, 0.850651f },
    { 0.309017f, 0.5f, 0.809017f }, { 0.525731f, 0.0f, 0.850651f },
    { 0.295242f, 0.0f, 0.955423f }, { 0.442863f, 0.238856f, 0.864188f },
    { 0.16246f, 0.262866f, 0.951056f }, { -0.681718f, 0.147621f, 0.716567f },
    { -0.809017f, 0.309017f, 0.5f }, { -0.587785f, 0.425325f, 0.688191f },
    { -0.850651f, 0.525731f, 0.0f }, { -0.864188f, 0.442863f, 0.238856f },
    { -0.716567f, 0.681718f, 0.147621f }, { -0.688191f, 0.587785f, 0.425325f },
    { -0.5f, 0.809017f, 0.309017f }, { -0.238856f, 0.864188f, 0.442863f },
    { -0.425325f, 0.688191f, 0.587785f }, { -0.716567f, 0.681718f, -0.147621f },
    { -0.5f, 0.809017f, -0.309017f }, { -0.525731f, 0.850651f, 0.0f },
    { 0.0f, 0.850651f, -0.525731f }, { -0.238856f, 0.864188f, -0.442863f },
    { 0.0f, 0.955423f, -0.295242f }, { -0.262866f, 0.951056f, -0.16246f },
    { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.955423f, 0.295242f },
    { -0.262866f, 0.951056f, 0.16246f }, { 0.238856f, 0.864188f, 0.442863f },
    { 0.262866f, 0.951056f, 0.16246f }, { 0.5f, 0.809017f, 0.309017f },
    { 0.238856f, 0.864188f, -0.442863f }, { 0.262866f, 0.951056f, -0.16246f },
    { 0.5f, 0.809017f, -0.309017f }, { 0.850651f, 0.525731f, 0.0f },
    { 0.716567f, 0.681718f, 0.147621f }, { 0.716567f, 0.681718f, -0.147621f },
    { 0.525731f, 0.850651f, 0.0f }, { 0.425325f, 0.688191f, 0.587785f },
    { 0.864188f, 0.442863f, 0.238856f }, { 0.688191f, 0.587785f, 0.425325f },
    { 0.809017f, 0.309017f, 0.5f }, { 0.681718f, 0.147621f, 0.716567f },
    { 0.587785f, 0.425325f, 0.688191f }, { 0.955423f, 0.295242f, 0.0f },
    { 1.0f, 0.0f, 0.0f }, { 0.951056f, 0.16246f, 0.262866f },
    { 0.850651f, -0.525731f, 0.0f }, { 0.955423f, -0.295242f, 0.0f },
    { 0.864188f, -0.442863f, 0.238856f }, { 0.951056f, -0.16246f, 0.262866f },
    { 0.809017f, -0.309017f, 0.5f }, { 0.681718f, -0.147621f, 0.716567f },
    { 0.850651f, 0.0f, 0.525731f }, { 0.864188f, 0.442863f, -0.238856f },
    { 0.809017f, 0.309017f, -0.5f }, { 0.951056f, 0.16246f, -0.262866f },
    { 0.525731f, 0.0f, -0.850651f }, { 0.681718f, 0.147621f, -0.716567f },
    { 0.681718f, -0.147621f, -0.716567f }, { 0.850651f, 0.0f, -0.525731f },
    { 0.809017f, -0.309017f, -0.5f }, { 0.864188f, -0.442863f, -0.238856f },
    { 0.951056f, -0.16246f, -0.262866f }, { 0.147621f, 0.716567f, -0.681718f },
    { 0.309017f, 0.5f, -0.809017f }, { 0.425325f, 0.688191f, -0.587785f },
    { 0.442863f, 0.238856f, -0.864188f }, { 0.587785f, 0.425325f, -0.688191f },
    { 0.688191f, 0.587785f, -0.425325f }, { -0.147621f, 0.716567f, -0.681718f },
    { -0.309017f, 0.5f, -0.809017f }, { 0.0f, 0.525731f, -0.850651f },
    { -0.525731f, 0.0f, -0.850651f }, { -0.442863f, 0.238856f, -0.864188f },
    { -0.295242f, 0.0f, -0.955423f }, { -0.16246f, 0.262866f, -0.951056f },
    { 0.0f, 0.0f, -1.0f }, { 0.295242f, 0.0f, -0.955423f },
    { 0.16246f, 0.262866f, -0.951056f }, { -0.442863f, -0.238856f, -0.864188f },
    { -0.309017f, -0.5f, -0.809017f }, { -0.16246f, -0.262866f, -0.951056f },
    { 0.0f, -0.850651f, -0.525731f }, { -0.147621f, -0.716567f, -0.681718f },
    { 0.147621f, -0.716567f, -0.681718f }, { 0.0f, -0.525731f, -0.850651f },
    { 0.309017f, -0.5f, -0.809017f }, { 0.442863f, -0.238856f, -0.864188f },
    { 0.16246f, -0.262866f, -0.951056f }, { 0.238856f, -0.864188f, -0.442863f },
    { 0.5f, -0.809017f, -0.309017f }, { 0.425325f, -0.688191f, -0.587785f },
    { 0.716567f, -0.681718f, -0.147621f }, { 0.688191f, -0.587785f, -0.425325f },
    { 0.587785f, -0.425325f, -0.688191f }, { 0.0f, -0.955423f, -0.295242f },
    { 0.0f, -1.0f, 0.0f }, { 0.262866f, -0.951056f, -0.16246f },
    { 0.0f, -0.850651f, 0.525731f }, { 0.0f, -0.955423f, 0.295242f },
    { 0.238856f, -0.864188f, 0.442863f }, { 0.262866f, -0.951056f, 0.16246f },
    { 0.5f, -0.809017f, 0.309017f }, { 0.716567f, -0.681718f, 0.147621f },
    { 0.525731f, -0.850651f, 0.0f }, { -0.238856f, -0.864188f, -0.442863f },
    { -0.5f, -0.809017f, -0.309017f }, { -0.262866f, -0.951056f, -0.16246f },
    { -0.850651f, -0.525731f, 0.0f }, { -0.716567f, -0.681718f, -0.147621f },
    { -0.716567f, -0.681718f, 0.147621f }, { -0.525731f, -0.850651f, 0.0f },
    { -0.5f, -0.809017f, 0.309017f }, { -0.238856f, -0.864188f, 0.442863f },
    { -0.262866f, -0.951056f, 0.16246f }, { -0.864188f, -0.442863f, 0.238856f },
    { -0.809017f, -0.309017f, 0.5f }, { -0.688191f, -0.587785f, 0.425325f },
    { -0.681718f, -0.147621f, 0.716567f }, { -0.442863f, -0.238856f, 0.864188f },
    { -0.587785f, -0.425325f, 0.688191f }, { -0.309017f, -0.5f, 0.809017f },
    { -0.147621f, -0.716567f, 0.681718f }, { -0.425325f, -0.688191f, 0.587785f },
    { -0.16246f, -0.262866f, 0.951056f }, { 0.442863f, -0.238856f, 0.864188f },
    { 0.16246f, -0.262866f, 0.951056f }, { 0.309017f, -0.5f, 0.809017f },
    { 0.147621f, -0.716567f, 0.681718f }, { 0.0f, -0.525731f, 0.850651f },
    { 0.425325f, -0.688191f, 0.587785f }, { 0.587785f, -0.425325f, 0.688191f },
    { 0.688191f, -0.587785f, 0.425325f }, { -0.955423f, 0.295242f, 0.0f },
    { -0.951056f, 0.16246f, 0.262866f }, { -1.0f, 0.0f, 0.0f },
    { -0.850651f, 0.0f, 0.525731f }, { -0.955423f, -0.295242f, 0.0f },
    { -0.951056f, -0.16246f, 0.262866f }, { -0.864188f, 0.442863f, -0.238856f },
    { -0.951056f, 0.16246f, -0.262866f }, { -0.809017f, 0.309017f, -0.5f },
    { -0.864188f, -0.442863f, -0.238856f }, { -0.951056f, -0.16246f, -0.262866f },
    { -0.809017f, -0.309017f, -0.5f }, { -0.681718f, 0.147621f, -0.716567f },
    { -0.681718f, -0.147621f, -0.716567f }, { -0.850651f, 0.0f, -0.525731f },
    { -0.688191f, 0.587785f, -0.425325f }, { -0.587785f, 0.425325f, -0.688191f },
    { -0.425325f, 0.688191f, -0.587785f }, { -0.425325f, -0.688191f, -0.587785f },
    { -0.587785f, -0.425325f, -0.688191f }, { -0.688191f, -0.587785f, -0.425325f },
};

static int g_randSeed;                                      /* 0x00529e48 */


/* random  0x0046cac0 */
float random( void )
{
    return ( float )( rand() / RAND_SCALE );
}

/* crandom  0x0046caf0 */
float crandom( void )
{
    return ( float )( random() * 2.0 - 1.0 );
}

/* RandGaussianPair  0x0046cb10 */
void RandGaussianPair( float *f0, float *f1 )
{
    float x;
    float y;
    float s;
    float scale;

    Assert( f0 );
    Assert( f1 );

    do
    {
        x = crandom();
        y = crandom();
        s = x * x + y * y;
    }
    while ( s > 1.0f );

    scale = I_sqrt( ( float )( ( float )log( s ) * -2.0 / s ) );

    *f0 = x * scale;
    *f1 = y * scale;
}

/* RandInt  0x0046cbe0 */
int RandInt( int *seed )
{
    *seed = *seed * RAND_MULTIPLIER + RAND_ADDEND;
    return ( unsigned int )( *seed / 65536 ) % 32768;
}

/* SpreadPointInCircle  0x0046cc20 */
void SpreadPointInCircle( float radiusSq, float angleFrac, vec2_t out )
{
    float radius;
    float s;
    float c;

    radius = I_sqrt( radiusSq );
    SinCos( angleFrac * TWO_PI, &s, &c );

    out[0] = radius * c;
    out[1] = radius * s;
}

/* SpreadPointOnSphere  0x0046cc80 */
void SpreadPointOnSphere( float heightFrac, float angleFrac, vec3_t out )
{
    float height;
    float radius;
    float s;
    float c;

    height = ( heightFrac + heightFrac ) - 1.0f;
    radius = I_sqrt( 1.0f - height * height );
    SinCos( angleFrac * TWO_PI, &s, &c );

    out[0] = radius * c;
    out[1] = radius * s;
    out[2] = height;
}

/* SpreadPointOnHemisphere  0x0046cd00 */
void SpreadPointOnHemisphere( float height, float angleFrac, vec3_t out )
{
    float radius;
    float s;
    float c;

    radius = I_sqrt( 1.0f - height * height );
    SinCos( angleFrac * TWO_PI, &s, &c );

    out[0] = radius * c;
    out[1] = radius * s;
    out[2] = height;
}

/* SpreadPointsInCircle  0x0046cd80 */
void SpreadPointsInCircle( unsigned int count, vec2_t *points, unsigned int stride )
{
    float        sinStep = SPREAD_SIN_STEP;
    float        cosStep = SPREAD_COS_STEP;
    float        step;
    float        frac;
    float        sinAngle;
    float        cosAngle;
    float        sinTemp;
    float        radius;
    unsigned int i;

    Assert( points );
    Assert( stride >= sizeof( vec2_t ) );

    step     = 1.0f / count;
    frac     = step * 0.5f;
    sinAngle = SPREAD_SIN_START;
    cosAngle = SPREAD_COS_START;

    for ( i = 0; i < count; i++ )
    {
        radius = I_sqrt( frac );

        ( *points )[0] = radius * cosAngle;
        ( *points )[1] = radius * sinAngle;

        frac = frac + step;

        sinTemp  = sinAngle;
        sinAngle = sinAngle * cosStep + cosAngle * sinStep;
        cosAngle = cosAngle * cosStep - sinTemp * sinStep;

        points = ( vec2_t * )( ( byte * )points + stride );
    }
}

/* SpreadPointsOnHemisphere  0x0046cea0 */
void SpreadPointsOnHemisphere( unsigned int count, vec3_t *points, unsigned int stride )
{
    float        sinStep = SPREAD_SIN_STEP;
    float        cosStep = SPREAD_COS_STEP;
    float        step;
    float        frac;
    float        sinAngle;
    float        cosAngle;
    float        sinTemp;
    float        height;
    float        radius;
    unsigned int i;

    Assert( points );
    Assert( stride >= sizeof( vec3_t ) );

    step     = 1.0f / count;
    frac     = step * 0.5f;
    sinAngle = 0.0f;
    cosAngle = 1.0f;

    for ( i = 0; i < count; i++ )
    {
        height = frac;
        radius = I_sqrt( 1.0f - height * height );

        ( *points )[0] = radius * cosAngle;
        ( *points )[1] = radius * sinAngle;
        ( *points )[2] = height;

        frac = frac + step;

        sinTemp  = sinAngle;
        sinAngle = sinAngle * cosStep + cosAngle * sinStep;
        cosAngle = cosAngle * cosStep - sinTemp * sinStep;

        points = ( vec3_t * )( ( byte * )points + stride );
    }
}

/* SpreadPointsOnSphere  0x0046cfe0 */
void SpreadPointsOnSphere( unsigned int count, vec3_t *points, unsigned int stride )
{
    float        sinStep = SPREAD_SIN_STEP;
    float        cosStep = SPREAD_COS_STEP;
    float        step;
    float        frac;
    float        sinAngle;
    float        cosAngle;
    float        sinTemp;
    float        height;
    float        radius;
    unsigned int i;

    Assert( points );
    Assert( stride >= sizeof( vec3_t ) );

    step     = 1.0f / count;
    frac     = step * 0.5f;
    sinAngle = 0.0f;
    cosAngle = 1.0f;

    for ( i = 0; i < count; i++ )
    {
        height = ( frac + frac ) - 1.0f;
        radius = I_sqrt( 1.0f - height * height );

        ( *points )[0] = radius * cosAngle;
        ( *points )[1] = radius * sinAngle;
        ( *points )[2] = height;

        frac = frac + step;

        sinTemp  = sinAngle;
        sinAngle = sinAngle * cosStep + cosAngle * sinStep;
        cosAngle = cosAngle * cosStep - sinTemp * sinStep;

        points = ( vec3_t * )( ( byte * )points + stride );
    }
}

/* DiffTrack  0x0046d120 */
float DiffTrack( float target, float current, float rate, float deltaTime )
{
    float diff;
    float step;

    diff = target - current;

    if ( diff > 0.0f )
        step = rate * deltaTime;
    else
        step = -rate * deltaTime;

    if ( I_fabs( diff ) > DIFFTRACK_EPSILON )
    {
        if ( I_fabs( diff ) < I_fabs( step ) )
            return target;

        return current + step;
    }

    return target;
}

/* AngleDiffTrack  0x0046d1c0 */
float AngleDiffTrack( float target, float current, float rate, float deltaTime )
{
    while ( target - current > 180.0f )
        target = target - 360.0f;

    while ( target - current < -180.0f )
        target = target + 360.0f;

    return AngleNormalize180( DiffTrack( target, current, rate, deltaTime ) );
}

/* DiffTrackSimple  0x0046d240 */
float DiffTrackSimple( float target, float current, float rate, float deltaTime )
{
    float diff;
    float step;

    diff = target - current;
    step = rate * diff * deltaTime;

    if ( I_fabs( diff ) > DIFFTRACK_EPSILON )
    {
        if ( I_fabs( diff ) < I_fabs( step ) )
            return target;

        return current + step;
    }

    return target;
}

/* AngleDiffTrackSimple  0x0046d2c0 */
float AngleDiffTrackSimple( float target, float current, float rate, float deltaTime )
{
    while ( target - current > 180.0f )
        target = target - 360.0f;

    while ( target - current < -180.0f )
        target = target + 360.0f;

    return AngleNormalize180( DiffTrackSimple( target, current, rate, deltaTime ) );
}

/* EvalKnots  0x0046d340 */
float EvalKnots( int knotCount, const vec2_t *knots, float fraction )
{
    float result;
    float adjustedFrac;
    int   knotIndex;

    result = -1.0f;

    Assert( knots );
    Assertx( knotCount >= 2, "(knotCount) = %i", knotCount );
    Assertx( fraction >= 0.0f && fraction <= 1.0f, "(fraction) = %g", fraction );
    Assertx( knots[knotCount - 1][0] == 1.0f, "(knots[knotCount - 1][0]) = %g",
             knots[knotCount - 1][0] );

    for ( knotIndex = 1; knotIndex < knotCount; knotIndex++ )
    {
        if ( fraction <= knots[knotIndex][0] )
        {
            adjustedFrac = ( fraction - knots[knotIndex - 1][0] ) /
                           ( knots[knotIndex][0] - knots[knotIndex - 1][0] );

            Assertx( adjustedFrac >= 0.0f && adjustedFrac <= 1.0f,
                     "(adjustedFrac) = %g", adjustedFrac );
            Assertx( knots[knotIndex - 1][1] >= 0.0f && knots[knotIndex - 1][1] <= 1.0f,
                     "(knots[knotIndex - 1][1]) = %g", knots[knotIndex - 1][1] );
            Assertx( knots[knotIndex][1] >= 0.0f && knots[knotIndex][1] <= 1.0f,
                     "(knots[knotIndex][1]) = %g", knots[knotIndex][1] );

            result = ( knots[knotIndex][1] - knots[knotIndex - 1][1] ) * adjustedFrac +
                     knots[knotIndex - 1][1];
            break;
        }
    }

    Assertx( result >= 0.0f && result <= 1.0f, "(result) = %g", result );

    return result;
}

/* Log2ForBitCount  0x0046d610 */
int Log2ForBitCount( int value )
{
    int bits;

    bits = 0;

    while ( value >>= 1 )
        bits++;

    return bits;
}

/* ACos  0x0046d640 */
float ACos( float x )
{
    float angle;

    angle = acos( x );

    if ( angle > PI )
        return PI;

    if ( angle < -PI )
        return PI;

    return angle;
}

/* ClampChar  0x0046d690 */
char ClampChar( int i )
{
    if ( i < -128 )
        return -128;

    if ( i > 127 )
        return 127;

    return ( char )i;
}

/* ClampShort  0x0046d6b0 */
short ClampShort( int i )
{
    if ( i < -32768 )
        return -32768;

    if ( i > 32767 )
        return 32767;

    return ( short )i;
}

/* DirToByte  0x0046d6e0 */
byte DirToByte( const vec3_t dir )
{
    byte  best;
    byte  i;
    float bestd;
    float d;

    if ( !dir )
        return 0;

    bestd = 0.0f;
    best  = 0;

    for ( i = 0; i < NUMVERTEXNORMALS; i++ )
    {
        d = Vec3Dot( dir, bytedirs[i] );

        if ( d > bestd )
        {
            bestd = d;
            best  = i;
        }
    }

    return best;
}

/* ByteToDir  0x0046d760 */
void ByteToDir( int b, vec3_t dir )
{
    if ( b < 0 || b >= NUMVERTEXNORMALS )
    {
        Vec3Copy( vec3_origin, dir );
        return;
    }

    Vec3Copy( bytedirs[b], dir );
}

/* VecNCompareEpsilon  0x0046d7a0 */
int VecNCompareEpsilon( const float *v0, const float *v1, float epsilon, int count )
{
    int i;

    for ( i = 0; i < count; i++ )
    {
        float epsilonSq = epsilon * epsilon;

        if ( ( v0[i] - v1[i] ) * ( v0[i] - v1[i] ) > epsilonSq )
            return 0;
    }

    return 1;
}

/* Vec3ConeDistance  0x0043f010 */
float Vec3ConeDistance( const vec3_t apex, const vec3_t dir, float cosHalfFov,
                        const vec3_t point )
{
    vec3_t delta;
    vec3_t perp;
    float  dot;
    float  perpLen;
    float  sinHalfFov;
    float  dist;

    Assert( cosHalfFov >= 0.0f );

    delta[0] = point[0] - apex[0];
    delta[1] = point[1] - apex[1];
    delta[2] = point[2] - apex[2];

    dot = dir[0] * delta[0] + dir[1] * delta[1] + dir[2] * delta[2];

    perp[0] = dir[0] * -dot + delta[0];
    perp[1] = dir[1] * -dot + delta[1];
    perp[2] = dir[2] * -dot + delta[2];

    perpLen = ( float )sqrt( ( float )( perp[0] * perp[0] + perp[1] * perp[1]
                                        + perp[2] * perp[2] ) );

    sinHalfFov = ( float )sqrt( ( float )( 1.0f - cosHalfFov * cosHalfFov ) );

    if ( dot > 0.0f && dot * cosHalfFov > sinHalfFov * perpLen )
        return ( float )sqrt( ( float )( delta[0] * delta[0] + delta[1] * delta[1]
                                         + delta[2] * delta[2] ) );

    dist = dot * sinHalfFov + perpLen * cosHalfFov;

    if ( -dist < 0.0f )
        return dist;

    return 0.0f;
}

/* Vec3MaxElement  0x004385e0 */
float Vec3MaxElement( const vec3_t v )
{
    float largest;
    float diff;

    diff = v[0] - v[1];

    largest = ( diff < 0.0f ) ? v[1] : v[0];

    diff = largest - v[2];

    if ( diff < 0.0f )
        largest = v[2];

    return largest;
}

/* Vec3DistanceSqToRay  0x0046d810 */
float Vec3DistanceSqToRay( const vec3_t point, const vec3_t start, const vec3_t dir, float length )
{
    vec3_t delta;
    vec3_t closest;
    float  dot;

    Vec3Sub( point, start, delta );
    dot = Vec3Dot( delta, dir );
    dot = I_fclamp( dot, 0.0f, length );
    Vec3Mad( start, dot, dir, closest );

    return Vec3DistanceSq( point, closest );
}

/* Vec2Distance  0x0046d8a0 */
float Vec2Distance( const vec2_t v0, const vec2_t v1 )
{
    vec2_t delta;

    Vec2Sub( v1, v0, delta );
    return Vec2Length( delta );
}

/* Vec2DistanceSq  0x0046d8d0 */
float Vec2DistanceSq( const vec2_t v0, const vec2_t v1 )
{
    vec2_t delta;

    Vec2Sub( v1, v0, delta );
    return delta[0] * delta[0] + delta[1] * delta[1];
}

/* VecLargestAxis  0x0046d910 */
int VecLargestAxis( const vec3_t dir )
{
    vec3_t sq;
    int    axis;

    Assert( dir );

    Vec3Mul( dir, dir, sq );

    axis = ( sq[0] < sq[1] ) ? 1 : 0;

    if ( sq[axis] < sq[2] )
        axis = 2;

    return axis;
}

/* GetProjectionAxes  0x0046d9a0 */
void GetProjectionAxes( const vec3_t normal, int *axisU, int *axisV )
{
    vec3_t sq;

    Vec3Mul( normal, normal, sq );

    if ( sq[2] >= sq[0] && sq[2] >= sq[1] )
    {
        if ( normal[2] > 0.0f )
        {
            *axisU = 0;
            *axisV = 1;
        }
        else
        {
            *axisU = 1;
            *axisV = 0;
        }
    }
    else if ( sq[1] >= sq[0] && sq[1] >= sq[2] )
    {
        if ( normal[1] > 0.0f )
        {
            *axisU = 2;
            *axisV = 0;
        }
        else
        {
            *axisU = 0;
            *axisV = 2;
        }
    }
    else
    {
        if ( normal[0] > 0.0f )
        {
            *axisU = 1;
            *axisV = 2;
        }
        else
        {
            *axisU = 2;
            *axisV = 1;
        }
    }
}

/* Vec2Normalize  0x0046dab0 */
float Vec2Normalize( vec2_t v )
{
    float length;
    float invLength;

    length    = I_sqrt( v[0] * v[0] + v[1] * v[1] );
    invLength = 1.0f / I_fsel( -length, 1.0f, length );

    v[0] = v[0] * invLength;
    v[1] = v[1] * invLength;

    return length;
}

/* Vec3NormalizeTo  0x0046db40 */
float Vec3NormalizeTo( const vec3_t in, vec3_t out )
{
    float length;
    float invLength;

    length    = I_sqrt( in[0] * in[0] + in[1] * in[1] + in[2] * in[2] );
    invLength = 1.0f / I_fsel( -length, 1.0f, length );

    out[0] = in[0] * invLength;
    out[1] = in[1] * invLength;
    out[2] = in[2] * invLength;

    return length;
}

/* Vec2NormalizeTo  0x0046dbe0 */
float Vec2NormalizeTo( const vec2_t in, vec2_t out )
{
    float length;
    float invLength;

    length    = I_sqrt( in[0] * in[0] + in[1] * in[1] );
    invLength = 1.0f / I_fsel( -length, 1.0f, length );

    out[0] = in[0] * invLength;
    out[1] = in[1] * invLength;

    return length;
}

/* Vec3MaxComponent  0x0046dc70 */
float Vec3MaxComponent( const vec3_t v )
{
    float m;

    m = I_fsel( v[0] - v[1], v[0], v[1] );
    return I_fsel( m - v[2], m, v[2] );
}

/* MatrixTransformVector33  0x0046dce0 */
void MatrixTransformVector33( const vec3_t in, const float mat[3][3], vec3_t out )
{
    Assert( in != out );

    out[0] = in[0] * mat[0][0] + in[1] * mat[0][1] + in[2] * mat[0][2];
    out[1] = in[0] * mat[1][0] + in[1] * mat[1][1] + in[2] * mat[1][2];
    out[2] = in[0] * mat[2][0] + in[1] * mat[2][1] + in[2] * mat[2][2];
}

/* MatrixTransposeTransformVector33  0x0046dda0 */
void MatrixTransposeTransformVector33( const vec3_t in, const float mat[3][3], vec3_t out )
{
    Assert( in != out );

    out[0] = in[0] * mat[0][0] + in[1] * mat[1][0] + in[2] * mat[2][0];
    out[1] = in[0] * mat[0][1] + in[1] * mat[1][1] + in[2] * mat[2][1];
    out[2] = in[0] * mat[0][2] + in[1] * mat[1][2] + in[2] * mat[2][2];
}

/* RotatePointAroundVector  0x0046de60 */
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
    float  m[3][3];
    float  im[3][3];
    float  zrot[3][3];
    float  tmpmat[3][3];
    float  rot[3][3];
    vec3_t vr;
    vec3_t vup;
    vec3_t vf;
    float  rad;
    int    i;

    Assert( dir[0] || dir[1] || dir[2] );

    vf[0] = dir[0];
    vf[1] = dir[1];
    vf[2] = dir[2];

    PerpendicularVector( dir, vr );
    Vec3Cross( vr, vf, vup );

    m[0][0] = vr[0];   m[0][1] = vup[0];  m[0][2] = vf[0];
    m[1][0] = vr[1];   m[1][1] = vup[1];  m[1][2] = vf[1];
    m[2][0] = vr[2];   m[2][1] = vup[2];  m[2][2] = vf[2];

    memcpy( im, m, sizeof( m ) );

    im[0][1] = vr[1];
    im[0][2] = vr[2];
    im[1][0] = vup[0];
    im[1][2] = vup[2];
    im[2][0] = vf[0];
    im[2][1] = vf[1];

    memset( zrot, 0, sizeof( zrot ) );
    zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0f;

    rad = degrees * DEG2RAD;
    Assert( !IS_NAN( rad ) );

    SinCos( rad, &zrot[0][1], &zrot[0][0] );

    Assert( !IS_NAN( zrot[0][1] ) );
    Assert( !IS_NAN( zrot[0][0] ) );

    zrot[1][0] = -zrot[0][1];
    zrot[1][1] = zrot[0][0];

    MatrixMultiply33( m, zrot, tmpmat );
    MatrixMultiply33( tmpmat, im, rot );

    for ( i = 0; i < 3; i++ )
        dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
}

/* AxisFromForwardAndRoll  0x0046e160 */
void AxisFromForwardAndRoll( float axis[3][3], float roll )
{
    vec3_t tmp;

    PerpendicularVector( axis[0], axis[1] );

    if ( roll != 0.0f )
    {
        Vec3Copy( axis[1], tmp );
        RotatePointAroundVector( axis[1], axis[0], tmp, roll );
    }

    Vec3Cross( axis[0], axis[1], axis[2] );
}

/* MakeNormalVectors  0x0046e1e0 */
void MakeNormalVectors( const vec3_t forward, vec3_t up, vec3_t right )
{
    PerpendicularVector( forward, right );
    Vec3Cross( forward, right, up );
}

/* MakeNormalVectors2  0x0046e210 */
void MakeNormalVectors2( const vec3_t forward, vec3_t up, vec3_t right )
{
    PerpendicularVector( forward, right );
    Vec3Cross( right, forward, up );
}

/* vectoyaw  0x0046e240 */
float vectoyaw( const vec3_t vec )
{
    float yaw;

    if ( vec[1] == 0.0f && vec[0] == 0.0f )
    {
        yaw = 0.0f;
    }
    else
    {
        yaw = ( float )atan2( vec[1], vec[0] ) * 180.0 / PI;
        yaw = I_fsel( yaw, 0.0f, 360.0f ) + yaw;
    }

    return yaw;
}

/* vectoyaw180  0x0046e2d0 */
float vectoyaw180( const vec3_t vec )
{
    float yaw;

    if ( vec[1] == 0.0f && vec[0] == 0.0f )
    {
        yaw = 0.0f;
    }
    else
    {
        yaw = ( float )atan2( vec[1], vec[0] ) * 180.0 / PI;
        Assert( yaw >= -180 );
        Assert( yaw <= 180 );
    }

    return yaw;
}

/* vectopitch  0x0046e3a0 */
float vectopitch( const vec3_t vec )
{
    float pitch;
    float forward;

    if ( vec[1] == 0.0f && vec[0] == 0.0f )
    {
        pitch = I_fsel( -vec[2], 90.0f, 270.0f );
    }
    else
    {
        forward = I_sqrt( vec[0] * vec[0] + vec[1] * vec[1] );
        pitch   = ( float )atan2( vec[2], forward ) * -180.0 / PI;
        pitch   = I_fsel( pitch, 0.0f, 360.0f ) + pitch;
    }

    return pitch;
}

/* vectopitch_no360  0x0046e480 */
float vectopitch_no360( const vec3_t vec )
{
    float pitch;
    float forward;

    if ( vec[1] == 0.0f && vec[0] == 0.0f )
    {
        pitch = I_fsel( -vec[2], 90.0f, -90.0f );
    }
    else
    {
        forward = I_sqrt( vec[0] * vec[0] + vec[1] * vec[1] );
        pitch   = ( float )atan2( vec[2], forward ) * -180.0 / PI;
    }

    return pitch;
}

/* vectoangles  0x0046e540 */
void vectoangles( const vec3_t vec, vec3_t angles )
{
    float yaw;
    float pitch;
    float forward;

    if ( vec[1] == 0.0f && vec[0] == 0.0f )
    {
        yaw   = 0.0f;
        pitch = I_fsel( -vec[2], 90.0f, 270.0f );
    }
    else
    {
        yaw = ( float )atan2( vec[1], vec[0] ) * 180.0 / PI;
        yaw = I_fsel( yaw, 0.0f, 360.0f ) + yaw;

        forward = I_sqrt( vec[0] * vec[0] + vec[1] * vec[1] );
        pitch   = ( float )atan2( vec[2], forward ) * -180.0 / PI;
        pitch   = I_fsel( pitch, 0.0f, 360.0f ) + pitch;
    }

    angles[0] = pitch;
    angles[1] = yaw;
    angles[2] = 0.0f;
}

/* QuatToAngles  0x0046e690 */
void QuatToAngles( const vec4_t quat, vec3_t angles )
{
    float axis[3][3];

    UnitQuatToAxis( quat, axis );
    AxisToAngles( ( const vec3_t * )axis, angles );
}

/* vectoangles_alt  0x0046e6c0 */
void vectoangles_alt( const vec3_t vec, vec3_t angles )
{
    float yaw;
    float pitch;
    float forward;

    if ( vec[1] == 0.0f && vec[0] == 0.0f )
    {
        yaw   = 0.0f;
        pitch = I_fsel( -vec[2], 90.0f, -90.0f );
    }
    else
    {
        yaw     = ( float )atan2( vec[1], vec[0] ) * 180.0 / PI;
        forward = I_sqrt( vec[0] * vec[0] + vec[1] * vec[1] );
        pitch   = ( float )atan2( vec[2], forward ) * -180.0 / PI;
    }

    angles[0] = pitch;
    angles[1] = yaw;
    angles[2] = 0.0f;
}

/* YawVectors  0x0046e7c0 */
void YawVectors( float yaw, vec3_t forward, vec3_t right )
{
    float rad;
    float s;
    float c;

    rad = yaw * DEG2RAD;
    SinCos( rad, &s, &c );

    if ( forward )
    {
        forward[0] = c;
        forward[1] = s;
        forward[2] = 0.0f;
    }

    if ( right )
    {
        right[0] = s;
        right[1] = -c;
        right[2] = 0.0f;
    }
}

/* YawVectors2D  0x0046e830 */
void YawVectors2D( float yaw, vec2_t forward, vec2_t right )
{
    float rad;
    float s;
    float c;

    rad = yaw * DEG2RAD;
    SinCos( rad, &s, &c );

    if ( forward )
    {
        forward[0] = c;
        forward[1] = s;
    }

    if ( right )
    {
        right[0] = s;
        right[1] = -c;
    }
}

/* PerpendicularVector  0x0046e890 */
void PerpendicularVector( const vec3_t src, vec3_t dst )
{
    vec3_t sq;
    int    minAxis;
    float  negComp;

    Assertx( Vec3IsNormalized( src ), "(%g %g %g) len %g",
             src[0], src[1], src[2], Vec3Length( src ) );

    sq[0] = src[0] * src[0];
    sq[1] = src[1] * src[1];
    sq[2] = src[2] * src[2];

    minAxis = ( sq[1] < sq[0] ) ? 1 : 0;

    if ( sq[2] < sq[minAxis] )
        minAxis = 2;

    negComp = -src[minAxis];
    Vec3Scale( src, negComp, dst );
    dst[minAxis] = dst[minAxis] + 1.0f;
    Vec3Normalize( dst );
}

/* TriangleNormal  0x0046e9c0 */
void TriangleNormal( const vec3_t p0, const vec3_t p1, const vec3_t p2, vec3_t normal )
{
    vec3_t d0;
    vec3_t d1;

    Vec3Sub( p0, p1, d0 );
    Vec3Normalize( d0 );
    Vec3Sub( p0, p2, d1 );
    Vec3Normalize( d1 );
    Vec3Cross( d0, d1, normal );
    Vec3Normalize( normal );
}

/* ProjectPointOntoVector  0x0046ea30 */
void ProjectPointOntoVector( const vec3_t point, const vec3_t vStart, const vec3_t vEnd, vec3_t out )
{
    vec3_t seg;
    vec3_t delta;
    float  dot;
    float  segDot;

    Vec3Sub( vEnd, vStart, seg );
    Vec3Sub( point, vStart, delta );

    dot    = Vec3Dot( delta, seg );
    segDot = Vec3Dot( seg, seg );

    if ( segDot != 0.0f )
        Vec3Mad( vStart, dot / segDot, seg, out );
    else
        Vec3Copy( vStart, out );
}

/* PointLineDistSq  0x0046ead0 */
float PointLineDistSq( const vec3_t point, const vec3_t start, const vec3_t end )
{
    vec3_t seg;
    vec3_t delta;
    vec3_t perp;
    float  dot;
    float  segDot;

    Vec3Sub( end, start, seg );
    Vec3Sub( point, start, delta );

    dot    = Vec3Dot( delta, seg );
    segDot = Vec3Dot( seg, seg );

    Assert( segDot );

    Vec3Mad( delta, -( dot / segDot ), seg, perp );

    return Vec3Dot( perp, perp );
}

/* Vec3DistanceSqToBounds  0x0046eb90 */
float Vec3DistanceSqToBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs )
{
    float dist;
    float d;
    int   i;

    dist = 0.0f;

    for ( i = 0; i < 3; i++ )
    {
        d = mins[i] - point[i];

        if ( d > 0.0f )
        {
            dist = d * d + dist;
        }
        else
        {
            d = point[i] - maxs[i];

            if ( d > 0.0f )
                dist = d * d + dist;
        }
    }

    return dist;
}

/* PointLineDistSq2D  0x0046ec20 */
float PointLineDistSq2D( const vec2_t point, const vec2_t start, const vec2_t end )
{
    vec2_t seg;
    vec2_t delta;
    vec2_t perp;
    float  dot;
    float  segDot;

    Vec2Sub( end, start, seg );
    Vec2Sub( point, start, delta );

    dot    = Vec2Dot( delta, seg );
    segDot = Vec2Dot( seg, seg );

    Assert( segDot );

    Vec2Mad( delta, -( dot / segDot ), seg, perp );

    return Vec2Dot( perp, perp );
}

/* PointLineDirDistSq  0x0046ece0 */
float PointLineDirDistSq( const vec3_t point, const vec3_t linePoint, const vec3_t lineDir )
{
    vec3_t delta;
    float  dot;

    Assert( Vec3IsNormalized( lineDir ) );

    Vec3Sub( point, linePoint, delta );
    dot = Vec3Dot( delta, lineDir );
    Vec3Mad( delta, -dot, lineDir, delta );

    return Vec3LengthSq( delta );
}

/* PointOnLineDir  0x0046ed70 */
void PointOnLineDir( const vec3_t point, const vec3_t linePoint, const vec3_t lineDir, vec3_t out )
{
    vec3_t delta;
    float  dot;

    Assert( Vec3IsNormalized( lineDir ) );

    Vec3Sub( point, linePoint, delta );
    dot = Vec3Dot( delta, lineDir );
    Vec3Mad( linePoint, dot, lineDir, out );
}

/* ClosestPointsBetweenLines  0x0046edf0 */
void ClosestPointsBetweenLines( const vec3_t p1, const vec3_t dir1,
                                const vec3_t p2, const vec3_t dir2,
                                float *outT1, float *outT2 )
{
    vec3_t r;
    float  dir1LenSq;
    float  dir2LenSq;
    float  dir1Dot2;
    float  dot1r;
    float  dot2r;
    float  rLenSq;
    float  denom;
    float  invDenom;
    float  epsilon;

    Vec3Sub( p1, p2, r );

    dir1LenSq = Vec3LengthSq( dir1 );
    dir2LenSq = Vec3LengthSq( dir2 );
    dir1Dot2  = -Vec3Dot( dir1, dir2 );
    dot1r     = Vec3Dot( dir1, r );
    rLenSq    = Vec3Dot( r, r );

    denom   = dir1LenSq * dir2LenSq - dir1Dot2 * dir1Dot2;
    epsilon = 0.0001f;

    if ( denom * denom > I_fabs( dir1LenSq * dir1Dot2 ) * epsilon )
    {
        dot2r    = -Vec3Dot( dir2, r );
        invDenom = 1.0f / denom;

        *outT1 = ( dir1Dot2 * dot2r - dir2LenSq * dot1r ) * invDenom;
        *outT2 = ( dir1Dot2 * dot1r - dir1LenSq * dot2r ) * invDenom;
    }
    else
    {
        Assert( dir1LenSq > 0.00001f );

        *outT1 = -dot1r / dir1LenSq;
        *outT2 = 0.0f;
    }
}

/* MatrixIdentity33  0x0046ef50 */
void MatrixIdentity33( float mtx[3][3] )
{
    Assert( mtx );

    memset( mtx, 0, sizeof( float ) * 9 );

    mtx[0][0] = 1.0f;
    mtx[1][1] = 1.0f;
    mtx[2][2] = 1.0f;
}

/* MatrixIdentity44  0x0046efb0 */
void MatrixIdentity44( float mtx[4][4] )
{
    Assert( mtx );

    memcpy( mtx, &g_identityMatrix44, sizeof( g_identityMatrix44 ) );
}

/* MatrixSetTransform44  0x0046eff0 */
void MatrixSetTransform44( float mtx[4][4], const vec3_t origin, const float axis[3][3], float scale )
{
    mtx[0][0] = axis[0][0] * scale;
    mtx[0][1] = axis[0][1] * scale;
    mtx[0][2] = axis[0][2] * scale;
    mtx[0][3] = 0.0f;

    mtx[1][0] = axis[1][0] * scale;
    mtx[1][1] = axis[1][1] * scale;
    mtx[1][2] = axis[1][2] * scale;
    mtx[1][3] = 0.0f;

    mtx[2][0] = axis[2][0] * scale;
    mtx[2][1] = axis[2][1] * scale;
    mtx[2][2] = axis[2][2] * scale;
    mtx[2][3] = 0.0f;

    mtx[3][0] = origin[0];
    mtx[3][1] = origin[1];
    mtx[3][2] = origin[2];
    mtx[3][3] = 1.0f;
}

/* MatrixMultiply33  0x0046f0c0 */
void MatrixMultiply33( const float in1[3][3], const float in2[3][3], float out[3][3] )
{
    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];

    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];

    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

/* MatrixPreMultiply33  0x0046f260 */
void MatrixPreMultiply33( const float in1[3][3], float mtx[3][3] )
{
    vec3_t row0;
    vec3_t row1;

    row0[0] = in1[0][0] * mtx[0][0] + in1[0][1] * mtx[1][0] + in1[0][2] * mtx[2][0];
    row0[1] = in1[0][0] * mtx[0][1] + in1[0][1] * mtx[1][1] + in1[0][2] * mtx[2][1];
    row0[2] = in1[0][0] * mtx[0][2] + in1[0][1] * mtx[1][2] + in1[0][2] * mtx[2][2];

    row1[0] = in1[1][0] * mtx[0][0] + in1[1][1] * mtx[1][0] + in1[1][2] * mtx[2][0];
    row1[1] = in1[1][0] * mtx[0][1] + in1[1][1] * mtx[1][1] + in1[1][2] * mtx[2][1];
    row1[2] = in1[1][0] * mtx[0][2] + in1[1][1] * mtx[1][2] + in1[1][2] * mtx[2][2];

    mtx[2][0] = in1[2][0] * mtx[0][0] + in1[2][1] * mtx[1][0] + in1[2][2] * mtx[2][0];
    mtx[2][1] = in1[2][0] * mtx[0][1] + in1[2][1] * mtx[1][1] + in1[2][2] * mtx[2][1];
    mtx[2][2] = in1[2][0] * mtx[0][2] + in1[2][1] * mtx[1][2] + in1[2][2] * mtx[2][2];

    Vec3Copy( row0, mtx[0] );
    Vec3Copy( row1, mtx[1] );
}

/* MatrixMultiply34  0x0046f420 */
void MatrixMultiply34( const float in1[3][4], const float in2[3][4], float out[3][4] )
{
    Assert( in1 != out );
    Assert( in2 != out );

    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
    out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];

    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
    out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];

    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
    out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
}

/* MatrixMultiply43  0x0046f6b0 */
void MatrixMultiply43( const float in1[4][3], const float in2[4][3], float out[4][3] )
{
    Assert( in1 != out );
    Assert( in2 != out );

    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];

    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];

    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];

    out[3][0] = in1[3][0] * in2[0][0] + in1[3][1] * in2[1][0] + in1[3][2] * in2[2][0] + in2[3][0];
    out[3][1] = in1[3][0] * in2[0][1] + in1[3][1] * in2[1][1] + in1[3][2] * in2[2][1] + in2[3][1];
    out[3][2] = in1[3][0] * in2[0][2] + in1[3][1] * in2[1][2] + in1[3][2] * in2[2][2] + in2[3][2];
}

/* MatrixMultiply44  0x0046f940 */
void MatrixMultiply44( const float in1[4][4], const float in2[4][4], float out[4][4] )
{
    Assert( in1 != out );
    Assert( in2 != out );

    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0] + in1[0][3] * in2[3][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1] + in1[0][3] * in2[3][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2] + in1[0][3] * in2[3][2];
    out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3] * in2[3][3];

    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0] + in1[1][3] * in2[3][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1] + in1[1][3] * in2[3][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2] + in1[1][3] * in2[3][2];
    out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3] * in2[3][3];

    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0] + in1[2][3] * in2[3][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1] + in1[2][3] * in2[3][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2] + in1[2][3] * in2[3][2];
    out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3] * in2[3][3];

    out[3][0] = in1[3][0] * in2[0][0] + in1[3][1] * in2[1][0] + in1[3][2] * in2[2][0] + in1[3][3] * in2[3][0];
    out[3][1] = in1[3][0] * in2[0][1] + in1[3][1] * in2[1][1] + in1[3][2] * in2[2][1] + in1[3][3] * in2[3][1];
    out[3][2] = in1[3][0] * in2[0][2] + in1[3][1] * in2[1][2] + in1[3][2] * in2[2][2] + in1[3][3] * in2[3][2];
    out[3][3] = in1[3][0] * in2[0][3] + in1[3][1] * in2[1][3] + in1[3][2] * in2[2][3] + in1[3][3] * in2[3][3];
}

/* MatrixTranspose33  0x0046fd50 */
void MatrixTranspose33( const float in[3][3], float out[3][3] )
{
    Assert( in != out );

    out[0][0] = in[0][0];  out[0][1] = in[1][0];  out[0][2] = in[2][0];
    out[1][0] = in[0][1];  out[1][1] = in[1][1];  out[1][2] = in[2][1];
    out[2][0] = in[0][2];  out[2][1] = in[1][2];  out[2][2] = in[2][2];
}

/* MatrixTranspose44  0x0046fdf0 */
void MatrixTranspose44( const float in[4][4], float out[4][4] )
{
    Assert( in != out );

    out[0][0] = in[0][0];  out[0][1] = in[1][0];  out[0][2] = in[2][0];  out[0][3] = in[3][0];
    out[1][0] = in[0][1];  out[1][1] = in[1][1];  out[1][2] = in[2][1];  out[1][3] = in[3][1];
    out[2][0] = in[0][2];  out[2][1] = in[1][2];  out[2][2] = in[2][2];  out[2][3] = in[3][2];
    out[3][0] = in[0][3];  out[3][1] = in[1][3];  out[3][2] = in[2][3];  out[3][3] = in[3][3];
}

/* MatrixInverse33  0x0046fee0 */
void MatrixInverse33( const float in[3][3], float out[3][3] )
{
    float det;

    Assert( in != out );

    det = ( in[1][2] * in[0][1] - in[1][1] * in[0][2] ) * in[2][0] +
          ( ( in[2][2] * in[1][1] - in[2][1] * in[1][2] ) * in[0][0] -
            ( in[2][2] * in[0][1] - in[2][1] * in[0][2] ) * in[1][0] );

    Assert( det );

    det = 1.0f / det;

    out[0][0] =  ( in[2][2] * in[1][1] - in[2][1] * in[1][2] ) * det;
    out[0][1] = -( in[2][2] * in[0][1] - in[2][1] * in[0][2] ) * det;
    out[0][2] =  ( in[1][2] * in[0][1] - in[1][1] * in[0][2] ) * det;

    out[1][0] = -( in[2][2] * in[1][0] - in[2][0] * in[1][2] ) * det;
    out[1][1] =  ( in[2][2] * in[0][0] - in[2][0] * in[0][2] ) * det;
    out[1][2] = -( in[1][2] * in[0][0] - in[1][0] * in[0][2] ) * det;

    out[2][0] =  ( in[2][1] * in[1][0] - in[2][0] * in[1][1] ) * det;
    out[2][1] = -( in[2][1] * in[0][0] - in[2][0] * in[0][1] ) * det;
    out[2][2] =  ( in[1][1] * in[0][0] - in[1][0] * in[0][1] ) * det;
}

/* MatrixInverseOrthonormal43  0x004700f0 */
void MatrixInverseOrthonormal43( const float in[4][3], float out[4][3] )
{
    vec3_t negOrigin;

    MatrixTranspose33( ( const float ( * )[3] )in, ( float ( * )[3] )out );
    Vec3Sub( vec3_origin, in[3], negOrigin );
    MatrixTransformVector( negOrigin, ( const float ( * )[3] )out, out[3] );
}

/* MatrixInverse44  0x00470140 */
void MatrixInverse44( const float mat[4][4], float dst[4][4] )
{
    float col0[4], col1[4], col2[4], col3[4];
    float m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11;
    float det;
    int   i;

    Assert( mat != dst );

    for ( i = 0; i < 4; i++ )
    {
        col0[i] = mat[i][0];
        col1[i] = mat[i][1];
        col2[i] = mat[i][2];
        col3[i] = mat[i][3];
    }

    m0  = col2[2] * col3[3];
    m1  = col2[3] * col3[2];
    m2  = col2[1] * col3[3];
    m3  = col2[3] * col3[1];
    m4  = col2[1] * col3[2];
    m5  = col2[2] * col3[1];
    m6  = col2[0] * col3[3];
    m7  = col2[3] * col3[0];
    m8  = col2[0] * col3[2];
    m9  = col2[2] * col3[0];
    m10 = col2[0] * col3[1];
    m11 = col2[1] * col3[0];

    dst[0][0] = m4 * col1[3] + m3 * col1[2] + m0 * col1[1];
    dst[0][0] = dst[0][0] - ( m5 * col1[3] + m2 * col1[2] + m1 * col1[1] );
    dst[0][1] = m9 * col1[3] + m6 * col1[2] + m1 * col1[0];
    dst[0][1] = dst[0][1] - ( m8 * col1[3] + m7 * col1[2] + m0 * col1[0] );
    dst[0][2] = m10 * col1[3] + m7 * col1[1] + m2 * col1[0];
    dst[0][2] = dst[0][2] - ( m11 * col1[3] + m6 * col1[1] + m3 * col1[0] );
    dst[0][3] = m11 * col1[2] + m8 * col1[1] + m5 * col1[0];
    dst[0][3] = dst[0][3] - ( m10 * col1[2] + m9 * col1[1] + m4 * col1[0] );

    dst[1][0] = m5 * col0[3] + m2 * col0[2] + m1 * col0[1];
    dst[1][0] = dst[1][0] - ( m4 * col0[3] + m3 * col0[2] + m0 * col0[1] );
    dst[1][1] = m8 * col0[3] + m7 * col0[2] + m0 * col0[0];
    dst[1][1] = dst[1][1] - ( m9 * col0[3] + m6 * col0[2] + m1 * col0[0] );
    dst[1][2] = m11 * col0[3] + m6 * col0[1] + m3 * col0[0];
    dst[1][2] = dst[1][2] - ( m10 * col0[3] + m7 * col0[1] + m2 * col0[0] );
    dst[1][3] = m10 * col0[2] + m9 * col0[1] + m4 * col0[0];
    dst[1][3] = dst[1][3] - ( m11 * col0[2] + m8 * col0[1] + m5 * col0[0] );

    m0  = col0[2] * col1[3];
    m1  = col0[3] * col1[2];
    m2  = col0[1] * col1[3];
    m3  = col0[3] * col1[1];
    m4  = col0[1] * col1[2];
    m5  = col0[2] * col1[1];
    m6  = col0[0] * col1[3];
    m7  = col0[3] * col1[0];
    m8  = col0[0] * col1[2];
    m9  = col0[2] * col1[0];
    m10 = col0[0] * col1[1];
    m11 = col0[1] * col1[0];

    dst[2][0] = m4 * col3[3] + m3 * col3[2] + m0 * col3[1];
    dst[2][0] = dst[2][0] - ( m5 * col3[3] + m2 * col3[2] + m1 * col3[1] );
    dst[2][1] = m9 * col3[3] + m6 * col3[2] + m1 * col3[0];
    dst[2][1] = dst[2][1] - ( m8 * col3[3] + m7 * col3[2] + m0 * col3[0] );
    dst[2][2] = m10 * col3[3] + m7 * col3[1] + m2 * col3[0];
    dst[2][2] = dst[2][2] - ( m11 * col3[3] + m6 * col3[1] + m3 * col3[0] );
    dst[2][3] = m11 * col3[2] + m8 * col3[1] + m5 * col3[0];
    dst[2][3] = dst[2][3] - ( m10 * col3[2] + m9 * col3[1] + m4 * col3[0] );

    dst[3][0] = m1 * col2[1] + m5 * col2[3] + m2 * col2[2];
    dst[3][0] = dst[3][0] - ( m3 * col2[2] + m0 * col2[1] + m4 * col2[3] );
    dst[3][1] = m7 * col2[2] + m0 * col2[0] + m8 * col2[3];
    dst[3][1] = dst[3][1] - ( m1 * col2[0] + m9 * col2[3] + m6 * col2[2] );
    dst[3][2] = m3 * col2[0] + m11 * col2[3] + m6 * col2[1];
    dst[3][2] = dst[3][2] - ( m7 * col2[1] + m2 * col2[0] + m10 * col2[3] );
    dst[3][3] = m9 * col2[1] + m4 * col2[0] + m10 * col2[2];
    dst[3][3] = dst[3][3] - ( m5 * col2[0] + m11 * col2[2] + m8 * col2[1] );

    det = col0[0] * dst[0][0] + col0[1] * dst[0][1] + col0[2] * dst[0][2] + col0[3] * dst[0][3];

    Assert( det );

    det = 1.0f / det;

    for ( i = 0; i < 16; i++ )
        ( ( float * )dst )[i] = ( ( float * )dst )[i] * det;
}

/* MatrixTransformVector44  0x00470730 */
void MatrixTransformVector44( const vec4_t vec, const float mat[4][4], vec4_t out )
{
    Assert( vec != out );

    out[0] = vec[0] * mat[0][0] + vec[1] * mat[1][0] + vec[2] * mat[2][0] + vec[3] * mat[3][0];
    out[1] = vec[0] * mat[0][1] + vec[1] * mat[1][1] + vec[2] * mat[2][1] + vec[3] * mat[3][1];
    out[2] = vec[0] * mat[0][2] + vec[1] * mat[1][2] + vec[2] * mat[2][2] + vec[3] * mat[3][2];
    out[3] = vec[0] * mat[0][3] + vec[1] * mat[1][3] + vec[2] * mat[2][3] + vec[3] * mat[3][3];
}

/* MatrixInverseTransformVector43  0x00470850 */
void MatrixInverseTransformVector43( const vec3_t in1, const float in2[3][3], vec3_t out )
{
    Assert( in1 != out );

    out[0] = in1[0] * in2[0][0] + in1[1] * in2[0][1] + in1[2] * in2[0][2];
    out[1] = in1[0] * in2[1][0] + in1[1] * in2[1][1] + in1[2] * in2[1][2];
    out[2] = in1[0] * in2[2][0] + in1[1] * in2[2][1] + in1[2] * in2[2][2];
}

/* MatrixTransformPoint43  0x00470910 */
void MatrixTransformPoint43( const vec3_t in1, const float in2[4][3], vec3_t out )
{
    Assert( in1 != out );

    out[0] = in1[0] * in2[0][0] + in1[1] * in2[1][0] + in1[2] * in2[2][0] + in2[3][0];
    out[1] = in1[0] * in2[0][1] + in1[1] * in2[1][1] + in1[2] * in2[2][1] + in2[3][1];
    out[2] = in1[0] * in2[0][2] + in1[1] * in2[1][2] + in1[2] * in2[2][2] + in2[3][2];
}

/* MatrixInverseTransformPoint43  0x004709e0 */
void MatrixInverseTransformPoint43( const vec3_t in1, const float in2[4][3], vec3_t out )
{
    vec3_t delta;

    Assert( in1 != out );

    Vec3Sub( in1, in2[3], delta );

    out[0] = in2[0][0] * delta[0] + in2[0][1] * delta[1] + in2[0][2] * delta[2];
    out[1] = in2[1][0] * delta[0] + in2[1][1] * delta[1] + in2[1][2] * delta[2];
    out[2] = in2[2][0] * delta[0] + in2[2][1] * delta[1] + in2[2][2] * delta[2];
}

/* MatrixTransformPointInPlace43  0x00470aa0 */
void MatrixTransformPointInPlace43( vec3_t inout, const float mat[4][3] )
{
    float x;
    float y;

    x = inout[0] * mat[0][0] + inout[1] * mat[1][0] + inout[2] * mat[2][0] + mat[3][0];
    y = inout[0] * mat[0][1] + inout[1] * mat[1][1] + inout[2] * mat[2][1] + mat[3][1];

    inout[2] = inout[0] * mat[0][2] + inout[1] * mat[1][2] + inout[2] * mat[2][2] + mat[3][2];
    inout[0] = x;
    inout[1] = y;
}

/* Vec2Rotate  0x00470b50 */
void Vec2Rotate( vec2_t v, float degrees )
{
    float s;
    float c;
    float x;

    SinCos( degrees * DEG2RAD, &s, &c );

    x    = v[0] * c - v[1] * s;
    v[1] = v[1] * c + v[0] * s;
    v[0] = x;
}

/* QuatConjugate  0x00470bc0 */
void QuatConjugate( const vec4_t quat, vec4_t out )
{
    out[0] = -quat[0];
    out[1] = -quat[1];
    out[2] = -quat[2];
    out[3] = quat[3];
}

/* QuatToAxis  0x00470c00 */
void QuatToAxis( const vec4_t quat, float axis[3][3] )
{
    float xx, yy, zz, ww;
    float xs, ys;
    float xy, xz, xw;
    float yz, yw;
    float zw;
    float magSqr;
    float s;

    xx = quat[0] * quat[0];
    yy = quat[1] * quat[1];
    zz = quat[2] * quat[2];
    ww = quat[3] * quat[3];

    magSqr = xx + yy + zz + ww;
    Assert( magSqr > 0.0f );

    s = 2.0 / magSqr;

    xx = xx * s;
    yy = yy * s;
    zz = zz * s;

    xs = s * quat[0];
    xy = xs * quat[1];
    xz = xs * quat[2];
    xw = xs * quat[3];

    ys = s * quat[1];
    yz = ys * quat[2];
    yw = ys * quat[3];

    zw = s * quat[2] * quat[3];

    axis[0][0] = 1.0f - ( yy + zz );
    axis[0][1] = xy + zw;
    axis[0][2] = xz - yw;

    axis[1][0] = xy - zw;
    axis[1][1] = 1.0f - ( xx + zz );
    axis[1][2] = yz + xw;

    axis[2][0] = xz + yw;
    axis[2][1] = yz - xw;
    axis[2][2] = 1.0f - ( xx + yy );
}

/* UnitQuatToAxis  0x00470d90 */
void UnitQuatToAxis( const vec4_t quat, float axis[3][3] )
{
    float x2, y2, z2;
    float xx, xy, xz, xw;
    float yy, yz, yw;
    float zz, zw;

    Assertx( Vec4IsNormalized( quat ), "%g %g %g %g", quat[0], quat[1], quat[2], quat[3] );

    x2 = quat[0] + quat[0];
    xx = x2 * quat[0];
    xy = x2 * quat[1];
    xz = x2 * quat[2];
    xw = x2 * quat[3];

    y2 = quat[1] + quat[1];
    yy = y2 * quat[1];
    yz = y2 * quat[2];
    yw = y2 * quat[3];

    z2 = quat[2] + quat[2];
    zw = z2 * quat[3];
    zz = z2 * quat[2];

    axis[0][0] = 1.0f - ( yy + zz );
    axis[0][1] = xy + zw;
    axis[0][2] = xz - yw;

    axis[1][0] = xy - zw;
    axis[1][1] = 1.0f - ( xx + zz );
    axis[1][2] = yz + xw;

    axis[2][0] = xz + yw;
    axis[2][1] = yz - xw;
    axis[2][2] = 1.0f - ( xx + yy );
}

/* UnitQuatToForward  0x00470f20 */
void UnitQuatToForward( const vec4_t quat, vec3_t forward )
{
    Assertx( Vec4IsNormalized( quat ), "%g %g %g %g", quat[0], quat[1], quat[2], quat[3] );

    forward[0] = 1.0f - ( quat[1] * quat[1] + quat[2] * quat[2] ) * 2.0;
    forward[1] = ( quat[0] * quat[1] + quat[2] * quat[3] ) * 2.0;
    forward[2] = ( quat[0] * quat[2] - quat[1] * quat[3] ) * 2.0;
}

/* QuatSlerp  0x00471010 */
void QuatSlerp( const vec4_t from, const vec4_t to, float t, vec4_t out )
{
    float    cosom;
    float    omega;
    float    sinom;
    float    scale0;
    float    scale1;
    qboolean flip;

    cosom = Vec4Dot( from, to );

    if ( cosom < 0.0f )
    {
        flip  = qtrue;
        cosom = cosom * -1.0;
    }
    else
    {
        flip = qfalse;
    }

    if ( cosom > SLERP_LINEAR_COS )
    {
        scale0 = 1.0f - t;
        scale1 = t;
    }
    else
    {
        omega  = acosf( cosom );
        sinom  = sinf( omega );
        scale0 = sinf( 1.0f - t * omega ) / sinom;
        scale1 = sinf( t * omega ) / sinom;
    }

    if ( flip )
    {
        out[0] = scale0 * from[0] + scale1 * to[0] * -1.0;
        out[1] = scale0 * from[1] + scale1 * to[1] * -1.0;
        out[2] = scale0 * from[2] + scale1 * to[2] * -1.0;
        out[3] = scale0 * from[3] + scale1 * to[3] * -1.0;
    }
    else
    {
        out[0] = scale0 * from[0] + scale1 * to[0];
        out[1] = scale0 * from[1] + scale1 * to[1];
        out[2] = scale0 * from[2] + scale1 * to[2];
        out[3] = scale0 * from[3] + scale1 * to[3];
    }
}

/* QuatRunningAveragePortion  0x004711d0 */
float QuatRunningAveragePortion( const vec4_t quat )
{
    float xx, yy, zz;
    float lengthSq;
    float invLengthSq;

    xx = quat[0] * quat[0];
    yy = quat[1] * quat[1];
    zz = quat[2] * quat[2];

    lengthSq = xx + yy + zz + quat[3] * quat[3];

    if ( lengthSq == 0.0f )
        return 0.0f;

    invLengthSq = 1.0f / lengthSq;

    xx = xx * invLengthSq;
    yy = yy * invLengthSq;
    zz = zz * invLengthSq;

    return xx + yy + zz;
}

/* SinSqDegrees  0x00471270 */
float SinSqDegrees( float degrees )
{
    float s;

    s = sin( ( float )( degrees * DEG2RAD ) );
    return s * s;
}

/* QuatDifferencePortion  0x004712b0 */
float QuatDifferencePortion( const vec4_t q0, const vec4_t q1 )
{
    vec4_t conj;
    vec4_t diff;

    QuatConjugate( q1, conj );
    QuatMultiply( q0, conj, diff );

    return QuatRunningAveragePortion( diff );
}

/* QuatToAngleDeg  0x004712f0 */
float QuatToAngleDeg( const vec2_t quat )
{
    float xx;
    float magSqr;
    float s;
    float c;
    float sn;

    xx     = quat[0] * quat[0];
    magSqr = quat[1] * quat[1] + xx;

    Assert( magSqr );

    s  = 2.0f / magSqr;
    c  = 1.0f - xx * s;
    sn = quat[0] * quat[1] * s;

    return ( float )atan2( sn, c ) * RAD2DEG;
}

/* AxisAngleToQuat  0x004713a0 */
void AxisAngleToQuat( float degrees, const vec3_t axis, vec4_t quat )
{
    float s;

    degrees = degrees * HALF_DEG2RAD;
    SinCos( degrees, &s, &quat[3] );
    Vec3Scale( axis, s, quat );
}

/* MatrixRotationX  0x004713f0 */
void MatrixRotationX( float mtx[3][3], float degrees )
{
    float s;
    float c;

    SinCos( degrees * DEG2RAD, &s, &c );

    mtx[0][0] = 1.0f;
    mtx[0][1] = 0.0f;
    mtx[0][2] = 0.0f;

    mtx[1][0] = 0.0f;
    mtx[1][1] = c;
    mtx[1][2] = -s;

    mtx[2][0] = 0.0f;
    mtx[2][1] = s;
    mtx[2][2] = c;
}

/* MatrixRotationY  0x00471470 */
void MatrixRotationY( float mtx[3][3], float degrees )
{
    float s;
    float c;

    SinCos( degrees * DEG2RAD, &s, &c );

    mtx[0][0] = c;
    mtx[0][1] = 0.0f;
    mtx[0][2] = s;

    mtx[1][0] = 0.0f;
    mtx[1][1] = 1.0f;
    mtx[1][2] = 0.0f;

    mtx[2][0] = -s;
    mtx[2][1] = 0.0f;
    mtx[2][2] = c;
}

/* MatrixRotationZ  0x004714f0 */
void MatrixRotationZ( float mtx[3][3], float degrees )
{
    float s;
    float c;

    SinCos( degrees * DEG2RAD, &s, &c );

    mtx[0][0] = c;
    mtx[0][1] = -s;
    mtx[0][2] = 0.0f;

    mtx[1][0] = s;
    mtx[1][1] = c;
    mtx[1][2] = 0.0f;

    mtx[2][0] = 0.0f;
    mtx[2][1] = 0.0f;
    mtx[2][2] = 1.0f;
}

/* MatrixPerspectiveProjection  0x00471570 */
void MatrixPerspectiveProjection( float mtx[4][4], float tanHalfFovX, float tanHalfFovY,
                                  float zNear, float zFar )
{
    Assert( mtx );
    Assertx( zNear > 0.0f, "%g, %g", zNear, 0.0f );
    Assertx( zFar > zNear, "%g, %g", zFar, zNear );

    memset( mtx, 0, sizeof( float ) * 16 );

    mtx[0][0] = 1.0f / tanHalfFovX;
    mtx[1][1] = 1.0f / tanHalfFovY;
    mtx[2][2] = -zFar / ( zNear - zFar );
    mtx[2][3] = 1.0f;
    mtx[3][2] = ( zNear * zFar ) / ( zNear - zFar );
}

/* MatrixPerspectiveProjectionInfinite  0x00471670 */
void MatrixPerspectiveProjectionInfinite( float mtx[4][4], float tanHalfFovX,
                                          float tanHalfFovY, float zNear )
{
    float scale;

    Assert( mtx );
    Assert( zNear > 0 );

    memset( mtx, 0, sizeof( float ) * 16 );

    scale = INFINITE_FAR_SCALE;

    mtx[0][0] = scale / tanHalfFovX;
    mtx[1][1] = scale / tanHalfFovY;
    mtx[2][2] = scale;
    mtx[2][3] = 1.0f;
    mtx[3][2] = -zNear * scale;
}

/* MatrixOrthoProjection  0x00471720 */
void MatrixOrthoProjection( float mtx[4][4], float width, float height, float depth )
{
    Assert( mtx );
    Assert( width != 0 );
    Assert( height != 0 );
    Assert( depth != 0 );

    memset( mtx, 0, sizeof( float ) * 16 );

    mtx[0][0] = 2.0 / width;
    mtx[1][1] = 2.0 / height;
    mtx[2][2] = 0.5 / depth;
    mtx[3][2] = 0.5f;
    mtx[3][3] = 1.0f;
}

/* MatrixWorldToCamera  0x00471830 */
void MatrixWorldToCamera( float mtx[4][4], const vec3_t origin, const float axis[3][3] )
{
    Assert( mtx );
    Assert( origin );
    Assert( axis );

    mtx[0][0] = -axis[1][0];
    mtx[1][0] = -axis[1][1];
    mtx[2][0] = -axis[1][2];
    mtx[3][0] = -( origin[0] * mtx[0][0] + origin[1] * mtx[1][0] + origin[2] * mtx[2][0] );

    mtx[0][1] = axis[2][0];
    mtx[1][1] = axis[2][1];
    mtx[2][1] = axis[2][2];
    mtx[3][1] = -( origin[0] * mtx[0][1] + origin[1] * mtx[1][1] + origin[2] * mtx[2][1] );

    mtx[0][2] = axis[0][0];
    mtx[1][2] = axis[0][1];
    mtx[2][2] = axis[0][2];
    mtx[3][2] = -( origin[0] * mtx[0][2] + origin[1] * mtx[1][2] + origin[2] * mtx[2][2] );

    mtx[0][3] = 0.0f;
    mtx[1][3] = 0.0f;
    mtx[2][3] = 0.0f;
    mtx[3][3] = 1.0f;
}

/* AnglesSubtract  0x004719d0 */
void AnglesSubtract( const vec3_t a0, const vec3_t a1, vec3_t out )
{
    out[0] = AngleSubtract( a0[0], a1[0] );
    out[1] = AngleSubtract( a0[1], a1[1] );
    out[2] = AngleSubtract( a0[2], a1[2] );
}

/* AngleNormalize360  0x00471a40 */
float AngleNormalize360( float angle )
{
    float frac;
    float wrapped;
    float excess;

    frac    = angle * ( 1.0f / 360.0f );
    wrapped = ( frac - floor( frac ) ) * 360.0f;
    excess  = wrapped - 360.0f;

    return I_fsel( excess, excess, wrapped );
}

/* AngleDelta  0x00471aa0 */
float AngleDelta( float a1, float a2 )
{
    return AngleNormalize180( a1 - a2 );
}

/* RadiusFromBounds  0x00471ac0 */
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs )
{
    return I_sqrt( RadiusFromBoundsSq( mins, maxs ) );
}

/* RadiusFromBounds2D  0x00471ae0 */
float RadiusFromBounds2D( const vec2_t mins, const vec2_t maxs )
{
    return I_sqrt( RadiusFromBounds2DSq( mins, maxs ) );
}

/* RadiusFromBoundsSq  0x00471b00 */
float RadiusFromBoundsSq( const vec3_t mins, const vec3_t maxs )
{
    vec3_t corner;
    float  a;
    float  b;
    int    i;

    for ( i = 0; i < 3; i++ )
    {
        a = I_fabs( mins[i] );
        b = I_fabs( maxs[i] );

        if ( b < a )
            b = a;

        corner[i] = b;
    }

    return Vec3LengthSq( corner );
}

/* RadiusFromBounds2DSq  0x00471b90 */
float RadiusFromBounds2DSq( const vec2_t mins, const vec2_t maxs )
{
    vec2_t corner;
    float  a;
    float  b;
    int    i;

    for ( i = 0; i < 2; i++ )
    {
        a = I_fabs( mins[i] );
        b = I_fabs( maxs[i] );

        if ( b < a )
            b = a;

        corner[i] = b;
    }

    return Vec2LengthSq( corner );
}

/* ExpandBoundsForOffset  0x00471c20 */
void ExpandBoundsForOffset( vec3_t mins, vec3_t maxs, const vec3_t offset )
{
    if ( offset[0] <= 0.0f )
        mins[0] = mins[0] + offset[0];
    else
        maxs[0] = maxs[0] + offset[0];

    if ( offset[1] <= 0.0f )
        mins[1] = mins[1] + offset[1];
    else
        maxs[1] = maxs[1] + offset[1];

    if ( offset[2] <= 0.0f )
        mins[2] = mins[2] + offset[2];
    else
        maxs[2] = maxs[2] + offset[2];
}

/* ExpandBoundsToCube  0x00471cd0 */
void ExpandBoundsToCube( vec3_t mins, vec3_t maxs )
{
    vec3_t size;
    float  wanted;
    float  half;

    Assert( maxs[0] >= mins[0] );
    Assert( maxs[1] >= mins[1] );
    Assert( maxs[2] >= mins[2] );

    Vec3Sub( maxs, mins, size );

    wanted = I_fmax( size[0], size[1] );

    if ( size[2] < wanted )
    {
        half = ( wanted - size[2] ) * 0.5f;
        mins[2] = mins[2] - half;
        maxs[2] = maxs[2] + half;
    }
}

/* ShrinkBoundsToCube  0x00471de0 */
void ShrinkBoundsToCube( vec3_t mins, vec3_t maxs )
{
    vec3_t size;
    float  half;

    Assertx( maxs[0] >= mins[0], "%g, %g", maxs[0], mins[0] );
    Assertx( maxs[1] >= mins[1], "%g, %g", maxs[1], mins[1] );
    Assertx( maxs[2] >= mins[2], "%g, %g", maxs[2], mins[2] );

    Vec3Sub( maxs, mins, size );

    if ( size[2] < size[0] )
    {
        half = ( size[0] - size[2] ) * 0.5f;
        mins[0] = mins[0] + half;
        maxs[0] = maxs[0] - half;
    }

    if ( size[2] < size[1] )
    {
        half = ( size[1] - size[2] ) * 0.5f;
        mins[1] = mins[1] + half;
        maxs[1] = maxs[1] - half;
    }
}

/* ClearBounds  0x00471f50 */
void ClearBounds( vec3_t mins, vec3_t maxs )
{
    Vec3Set( mins, MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD );
    Vec3Set( maxs, MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD );
}

/* BoundsAreCleared  0x00471fb0 */
bool BoundsAreCleared( const vec3_t mins, const vec3_t maxs )
{
    if ( mins[0] != MAX_WORLD_COORD )
        return false;

    if ( mins[1] != MAX_WORLD_COORD )
        return false;

    if ( mins[2] != MAX_WORLD_COORD )
        return false;

    if ( maxs[0] != MIN_WORLD_COORD )
        return false;

    if ( maxs[1] != MIN_WORLD_COORD )
        return false;

    if ( maxs[2] != MIN_WORLD_COORD )
        return false;

    return true;
}

/* ClearBounds2D  0x00472030 */
void ClearBounds2D( vec2_t mins, vec2_t maxs )
{
    Vec2Set( mins, MAX_WORLD_COORD, MAX_WORLD_COORD );
    Vec2Set( maxs, MIN_WORLD_COORD, MIN_WORLD_COORD );
}

/* AddPointToBounds  0x00472080 */
void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs )
{
    if ( v[0] < mins[0] )
        mins[0] = v[0];
    if ( maxs[0] < v[0] )
        maxs[0] = v[0];

    if ( v[1] < mins[1] )
        mins[1] = v[1];
    if ( maxs[1] < v[1] )
        maxs[1] = v[1];

    if ( v[2] < mins[2] )
        mins[2] = v[2];
    if ( maxs[2] < v[2] )
        maxs[2] = v[2];
}

/* AddPointToBounds2D  0x00472150 */
void AddPointToBounds2D( const vec2_t v, vec2_t mins, vec2_t maxs )
{
    if ( v[0] < mins[0] )
        mins[0] = v[0];
    if ( maxs[0] < v[0] )
        maxs[0] = v[0];

    if ( v[1] < mins[1] )
        mins[1] = v[1];
    if ( maxs[1] < v[1] )
        maxs[1] = v[1];
}

/* PointInBounds  0x004721e0 */
int PointInBounds( const vec3_t v, const vec3_t mins, const vec3_t maxs )
{
    Assert( v );
    Assert( mins );
    Assert( maxs );

    if ( v[0] < mins[0] || v[0] > maxs[0] )
        return 0;

    if ( v[1] < mins[1] || v[1] > maxs[1] )
        return 0;

    if ( v[2] < mins[2] || v[2] > maxs[2] )
        return 0;

    return 1;
}

/* PointInBounds2D  0x004722f0 */
int PointInBounds2D( const vec2_t v, const vec2_t mins, const vec2_t maxs )
{
    Assert( v );
    Assert( mins );
    Assert( maxs );

    if ( v[0] < mins[0] || v[0] > maxs[0] )
        return 0;

    if ( v[1] < mins[1] || v[1] > maxs[1] )
        return 0;

    return 1;
}

/* BoundsOverlap  0x004723d0 */
int BoundsOverlap( const vec3_t mins0, const vec3_t maxs0, const vec3_t mins1, const vec3_t maxs1 )
{
    if ( mins0[0] > maxs1[0] || mins1[0] > maxs0[0] )
        return 0;

    if ( mins0[1] > maxs1[1] || mins1[1] > maxs0[1] )
        return 0;

    if ( mins0[2] > maxs1[2] || mins1[2] > maxs0[2] )
        return 0;

    return 1;
}

/* BoundsOverlap2D  0x00472460 */
int BoundsOverlap2D( const vec2_t mins0, const vec2_t maxs0, const vec2_t mins1, const vec2_t maxs1 )
{
    if ( mins0[0] > maxs1[0] || mins1[0] > maxs0[0] )
        return 0;

    if ( mins0[1] > maxs1[1] || mins1[1] > maxs0[1] )
        return 0;

    return 1;
}

/* BoundsOverlapTolerance  0x004724d0 */
int BoundsOverlapTolerance( const vec3_t mins0, const vec3_t maxs0,
                            const vec3_t mins1, const vec3_t maxs1, float tolerance )
{
    if ( maxs1[0] + tolerance < mins0[0] || maxs0[0] + tolerance < mins1[0] )
        return 0;

    if ( maxs1[1] + tolerance < mins0[1] || maxs0[1] + tolerance < mins1[1] )
        return 0;

    if ( maxs1[2] + tolerance < mins0[2] || maxs0[2] + tolerance < mins1[2] )
        return 0;

    return 1;
}

/* BoundsOverlapTolerance2D  0x00472580 */
int BoundsOverlapTolerance2D( const vec2_t mins0, const vec2_t maxs0,
                              const vec2_t mins1, const vec2_t maxs1, float tolerance )
{
    if ( maxs1[0] + tolerance < mins0[0] || maxs0[0] + tolerance < mins1[0] )
        return 0;

    if ( maxs1[1] + tolerance < mins0[1] || maxs0[1] + tolerance < mins1[1] )
        return 0;

    return 1;
}

/* AddBoundsToBounds  0x004725f0 */
void AddBoundsToBounds( const vec3_t srcMins, const vec3_t srcMaxs, vec3_t dstMins, vec3_t dstMaxs )
{
    if ( srcMins[0] < dstMins[0] )
        dstMins[0] = srcMins[0];
    if ( dstMaxs[0] < srcMaxs[0] )
        dstMaxs[0] = srcMaxs[0];

    if ( srcMins[1] < dstMins[1] )
        dstMins[1] = srcMins[1];
    if ( dstMaxs[1] < srcMaxs[1] )
        dstMaxs[1] = srcMaxs[1];

    if ( srcMins[2] < dstMins[2] )
        dstMins[2] = srcMins[2];
    if ( dstMaxs[2] < srcMaxs[2] )
        dstMaxs[2] = srcMaxs[2];
}

/* AddBoundsToBounds2D  0x004726c0 */
void AddBoundsToBounds2D( const vec2_t srcMins, const vec2_t srcMaxs, vec2_t dstMins, vec2_t dstMaxs )
{
    if ( srcMins[0] < dstMins[0] )
        dstMins[0] = srcMins[0];
    if ( dstMaxs[0] < srcMaxs[0] )
        dstMaxs[0] = srcMaxs[0];

    if ( srcMins[1] < dstMins[1] )
        dstMins[1] = srcMins[1];
    if ( dstMaxs[1] < srcMaxs[1] )
        dstMaxs[1] = srcMaxs[1];
}

/* BoundsVolume  0x00472750 */
float BoundsVolume( const vec3_t mins, const vec3_t maxs )
{
    return ( maxs[0] - mins[0] ) * ( maxs[1] - mins[1] ) * ( maxs[2] - mins[2] );
}

/* MatrixTransformBounds  0x00472790 */
void MatrixTransformBounds( const vec3_t bounds[2], const vec3_t origin,
                            const float mat[3][3], vec3_t outBounds[2] )
{
    int i;
    int side;

    for ( i = 0; i < 3; i++ )
    {
        outBounds[0][i] = origin[i];
        outBounds[1][i] = origin[i];

        side = ( Float_GetBits( &mat[0][i] ) >= 0 ) ? 0 : 1;
        outBounds[0][i] = bounds[side][0] * mat[0][i] + outBounds[0][i];
        outBounds[1][i] = bounds[1 - side][0] * mat[0][i] + outBounds[1][i];

        side = ( Float_GetBits( &mat[1][i] ) >= 0 ) ? 0 : 1;
        outBounds[0][i] = bounds[side][1] * mat[1][i] + outBounds[0][i];
        outBounds[1][i] = bounds[1 - side][1] * mat[1][i] + outBounds[1][i];

        side = ( Float_GetBits( &mat[2][i] ) >= 0 ) ? 0 : 1;
        outBounds[0][i] = bounds[side][2] * mat[2][i] + outBounds[0][i];
        outBounds[1][i] = bounds[1 - side][2] * mat[2][i] + outBounds[1][i];
    }
}

/* AxisClear  0x00472940 */
void AxisClear( vec3_t axis[3] )
{
    axis[0][0] = 1.0f;
    axis[0][1] = 0.0f;
    axis[0][2] = 0.0f;

    axis[1][0] = 0.0f;
    axis[1][1] = 1.0f;
    axis[1][2] = 0.0f;

    axis[2][0] = 0.0f;
    axis[2][1] = 0.0f;
    axis[2][2] = 1.0f;
}

/* AxisCopy  0x00472990 */
void AxisCopy( const vec3_t from[3], vec3_t to[3] )
{
    Vec3Copy( from[0], to[0] );
    Vec3Copy( from[1], to[1] );
    Vec3Copy( from[2], to[2] );
}

/* AxisTranspose  0x004729e0 */
void AxisTranspose( const vec3_t in[3], vec3_t out[3] )
{
    Assert( in != out );

    out[0][0] = in[0][0];  out[0][1] = in[1][0];  out[0][2] = in[2][0];
    out[1][0] = in[0][1];  out[1][1] = in[1][1];  out[1][2] = in[2][1];
    out[2][0] = in[0][2];  out[2][1] = in[1][2];  out[2][2] = in[2][2];
}

/* AxisTransformScalars  0x00472a80 */
void AxisTransformScalars( const vec3_t axis[3], float x, float y, float z, vec3_t out )
{
    out[0] = x * axis[0][0] + y * axis[1][0] + z * axis[2][0];
    out[1] = x * axis[0][1] + y * axis[1][1] + z * axis[2][1];
    out[2] = x * axis[0][2] + y * axis[1][2] + z * axis[2][2];
}

/* AxisTransformVec3  0x00472b00 */
void AxisTransformVec3( const vec3_t axis[3], const vec3_t in, vec3_t out )
{
    out[0] = in[0] * axis[0][0] + in[1] * axis[1][0] + in[2] * axis[2][0];
    out[1] = in[0] * axis[0][1] + in[1] * axis[1][1] + in[2] * axis[2][1];
    out[2] = in[0] * axis[0][2] + in[1] * axis[1][2] + in[2] * axis[2][2];
}

/* YawToAxis  0x00472b90 */
void YawToAxis( float yaw, vec3_t axis[3] )
{
    vec3_t right;

    YawVectors( yaw, axis[0], right );

    axis[2][0] = 0.0f;
    axis[2][1] = 0.0f;
    axis[2][2] = 1.0f;

    Vec3Sub( vec3_origin, right, axis[1] );
}

/* AxisToAngles  0x00472bf0 */
void AxisToAngles( const vec3_t axis[3], vec3_t angles )
{
    vec3_t v;
    float  s;
    float  c;
    float  tmp;
    float  roll;
    float  adj;

    vectoangles( axis[0], angles );
    Vec3Copy( axis[1], v );

    SinCos( -angles[1] * DEG2RAD, &s, &c );
    tmp  = c * v[0] - s * v[1];
    v[1] = s * v[0] + c * v[1];

    SinCos( -angles[0] * DEG2RAD, &s, &c );
    v[0] = s * v[2] + c * tmp;
    v[2] = c * v[2] - s * tmp;

    roll = vectopitch_no360( v );

    if ( v[1] < 0.0f )
    {
        if ( roll < 0.0f )
            adj = 180.0f;
        else
            adj = -180.0f;

        angles[2] = roll + adj;
    }
    else
    {
        angles[2] = -roll;
    }
}

/* AxisToAnglesAlt  0x00472d10 */
void AxisToAnglesAlt( const vec3_t axis[3], vec3_t angles )
{
    vec3_t v;
    float  s;
    float  c;
    float  tmp;
    float  roll;
    float  adj;

    vectoangles_alt( axis[0], angles );
    Vec3Copy( axis[1], v );

    SinCos( -angles[1] * DEG2RAD, &s, &c );
    tmp  = c * v[0] - s * v[1];
    v[1] = s * v[0] + c * v[1];

    SinCos( -angles[0] * DEG2RAD, &s, &c );
    v[0] = s * v[2] + c * tmp;
    v[2] = c * v[2] - s * tmp;

    roll = vectopitch_no360( v );

    if ( v[1] < 0.0f )
    {
        if ( roll < 0.0f )
            adj = 180.0f;
        else
            adj = -180.0f;

        angles[2] = roll + adj;
    }
    else
    {
        angles[2] = -roll;
    }
}

/* PlaneIntersection3  0x00472e30 */
bool PlaneIntersection3( const vec4_t *planes[3], vec3_t outPoint )
{
    const float *p0;
    const float *p1;
    const float *p2;
    double       det;
    double       invDet;

    p0 = *planes[0];
    p1 = *planes[1];
    p2 = *planes[2];

    det = ( double )( ( p1[1] * p2[2] - p2[1] * p1[2] ) * p0[0] ) +
          ( double )( ( p2[1] * p0[2] - p0[1] * p2[2] ) * p1[0] ) +
          ( double )( ( p0[1] * p1[2] - p1[1] * p0[2] ) * p2[0] );

    if ( fabs( det ) < 0.001f )
        return false;

    invDet = 1.0 / det;

    outPoint[0] = ( ( double )( ( p1[1] * p2[2] - p2[1] * p1[2] ) * p0[3] ) +
                    ( double )( ( p2[1] * p0[2] - p0[1] * p2[2] ) * p1[3] ) +
                    ( double )( ( p0[1] * p1[2] - p1[1] * p0[2] ) * p2[3] ) ) * invDet;

    outPoint[1] = ( ( double )( ( p1[2] * p2[0] - p2[2] * p1[0] ) * p0[3] ) +
                    ( double )( ( p2[2] * p0[0] - p0[2] * p2[0] ) * p1[3] ) +
                    ( double )( ( p0[2] * p1[0] - p1[2] * p0[0] ) * p2[3] ) ) * invDet;

    outPoint[2] = ( ( double )( ( p1[0] * p2[1] - p2[0] * p1[1] ) * p0[3] ) +
                    ( double )( ( p2[0] * p0[1] - p0[0] * p2[1] ) * p1[3] ) +
                    ( double )( ( p0[0] * p1[1] - p1[0] * p0[1] ) * p2[3] ) ) * invDet;

    return true;
}

/* SnapPlaneIntersection  0x004730f0 */
void SnapPlaneIntersection( const vec4_t *planes[3], vec3_t point, float gridSize, float tolerance )
{
    vec3_t snapped;
    float  snappedCoord;
    float  maxError;
    float  maxAllowed;
    float  dist;
    int    i;
    int    grid;

    for ( i = 0; i < 3; i++ )
    {
        grid         = RoundFloatToInt( point[i] / gridSize );
        snappedCoord = ( float )grid * gridSize;

        if ( I_fabs( snappedCoord - point[i] ) < tolerance )
            snapped[i] = snappedCoord;
        else
            snapped[i] = point[i];
    }

    if ( Vec3Compare( snapped, point ) )
        return;

    maxError   = 0.0f;
    maxAllowed = tolerance;

    for ( i = 0; i < 3; i++ )
    {
        dist = I_fabs( Vec3Dot( *planes[i], snapped ) - ( *planes[i] )[3] );

        if ( maxError < dist )
            maxError = dist;

        dist = I_fabs( Vec3Dot( *planes[i], point ) - ( *planes[i] )[3] );

        if ( maxAllowed < dist )
            maxAllowed = dist;
    }

    if ( maxError < maxAllowed )
        Vec3Copy( snapped, point );
}

/* SnapPointToPlanes  0x00473290 */
void SnapPointToPlanes( const vec4_t *planeArray, int numPlanes, vec3_t point,
                        float gridSize, float tolerance )
{
    vec3_t snapped;
    float  snappedCoord;
    float  planeDist;
    float  maxError;
    float  maxAllowed;
    float  dist;
    int    i;
    int    grid;

    for ( i = 0; i < numPlanes; i++ )
    {
        planeDist = Vec3Dot( point, planeArray[i] ) - planeArray[i][3];

        if ( planeDist <= tolerance && planeDist >= -tolerance )
            Vec3Mad( point, -planeDist, planeArray[i], point );
    }

    for ( i = 0; i < 3; i++ )
    {
        grid         = RoundFloatToInt( point[i] / gridSize );
        snappedCoord = ( float )grid * gridSize;

        if ( I_fabs( snappedCoord - point[i] ) < tolerance )
            snapped[i] = snappedCoord;
        else
            snapped[i] = point[i];
    }

    if ( Vec3Compare( snapped, point ) )
        return;

    maxError   = 0.0f;
    maxAllowed = tolerance;

    for ( i = 0; i < numPlanes; i++ )
    {
        dist = I_fabs( Vec3Dot( planeArray[i], snapped ) - planeArray[i][3] );

        if ( maxError < dist )
            maxError = dist;

        dist = I_fabs( Vec3Dot( planeArray[i], snapped ) - planeArray[i][3] );

        if ( maxAllowed < dist )
            maxAllowed = dist;
    }

    if ( maxError < maxAllowed )
        Vec3Copy( snapped, point );
}

/* PointInPoly  0x004734b0 */
int PointInPoly( const vec3_t *points, int numPoints, int axisU, int axisV, const vec3_t point )
{
    vec2_t edge;
    vec2_t delta;
    int    i;
    int    prev;

    prev = numPoints - 1;

    for ( i = 0; i < numPoints; i++ )
    {
        edge[0] = points[i][axisV] - points[prev][axisV];
        edge[1] = points[prev][axisU] - points[i][axisU];

        delta[0] = point[axisU] - points[prev][axisU];
        delta[1] = point[axisV] - points[prev][axisV];

        if ( Vec2Dot( delta, edge ) < 0.0f )
            return 0;

        prev = i;
    }

    return 1;
}

/* PointInPolyEpsilon  0x00473590 */
int PointInPolyEpsilon( const vec3_t *points, int numPoints, int axisU, int axisV,
                        const vec3_t point, float epsilon )
{
    vec2_t edge;
    vec2_t delta;
    float  dot;
    int    i;
    int    prev;

    prev = numPoints - 1;

    for ( i = 0; i < numPoints; i++ )
    {
        edge[0] = points[i][axisV] - points[prev][axisV];
        edge[1] = points[prev][axisU] - points[i][axisU];

        delta[0] = point[axisU] - points[prev][axisU];
        delta[1] = point[axisV] - points[prev][axisV];

        dot = Vec2Dot( delta, edge );

        if ( dot < 0.0f && dot * dot > Vec2LengthSq( edge ) * epsilon )
            return 0;

        prev = i;
    }

    return 1;
}

/* PolyClipFraction  0x004736a0 */
int PolyClipFraction( const vec3_t *points, int numPoints, int axisU, int axisV,
                      const vec3_t start, const vec3_t end, float *outFrac )
{
    vec2_t edge;
    vec2_t deltaStart;
    vec2_t deltaEnd;
    float  frac;
    float  dStart;
    float  dEnd;
    int    found;
    int    i;
    int    prev;

    frac  = FLT_MAX;
    found = 0;
    prev  = numPoints - 1;

    for ( i = 0; i < numPoints; i++ )
    {
        edge[0] = points[i][axisV] - points[prev][axisV];
        edge[1] = points[prev][axisU] - points[i][axisU];

        deltaStart[0] = start[axisU] - points[prev][axisU];
        deltaStart[1] = start[axisV] - points[prev][axisV];

        dStart = Vec2Dot( deltaStart, edge );

        if ( dStart < 0.0f )
        {
            deltaEnd[0] = end[axisU] - points[prev][axisU];
            deltaEnd[1] = end[axisV] - points[prev][axisV];

            dEnd = Vec2Dot( deltaEnd, edge );

            if ( dEnd >= 0.0f )
            {
                frac  = I_fmin( dEnd / ( dEnd - dStart ), frac );
                found = 1;
            }
        }

        prev = i;
    }

    *outFrac = frac;
    return found;
}

/* PlaneFromPoints  0x00473820 */
int PlaneFromPoints( vec4_t plane, const vec3_t point0, const vec3_t point1, const vec3_t point2 )
{
    vec3_t d1;
    vec3_t d2;
    float  lenSq;
    float  len;

    Vec3Sub( point1, point0, d1 );
    Vec3Sub( point2, point0, d2 );
    Vec3Cross( d2, d1, plane );

    lenSq = Vec3LengthSq( plane );

    if ( lenSq < 2.0f )
    {
        if ( lenSq == 0.0f )
            return 0;

        if ( lenSq < Vec3LengthSq( d1 ) * Vec3LengthSq( d2 ) * 0.000001f )
        {
            Vec3Sub( point2, point1, d1 );
            Vec3Sub( point0, point1, d2 );
            Vec3Cross( d2, d1, plane );

            if ( lenSq < Vec3LengthSq( d1 ) * Vec3LengthSq( d2 ) * 0.000001f )
                return 0;
        }
    }

    len = I_sqrt( lenSq );

    plane[0] = plane[0] / len;
    plane[1] = plane[1] / len;
    plane[2] = plane[2] / len;
    plane[3] = Vec3Dot( point0, plane );

    return 1;
}

/* ProjectPointOnPlane  0x004739a0 */
void ProjectPointOnPlane( const vec3_t point, const vec3_t normal, vec3_t out )
{
    float dot;

    Assertx( Vec3IsNormalized( normal ), "(%g %g %g) len %g",
             normal[0], normal[1], normal[2], Vec3Length( normal ) );

    dot = -Vec3Dot( normal, point );
    Vec3Mad( point, dot, normal, out );
}

/* BoxOnPlaneSide  0x00473a50 */
__declspec( naked ) int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, const struct cplane_s *p )
{
    static int bops_initialized;                    /* 0x2b9256b8 */
    static int Ljmptab[8];                          /* 0x2b925698 */

    __asm
    {
        push ebx

        cmp bops_initialized, 1
        je  initialized
        mov bops_initialized, 1

        mov Ljmptab[0*4], offset Lcase0
        mov Ljmptab[1*4], offset Lcase1
        mov Ljmptab[2*4], offset Lcase2
        mov Ljmptab[3*4], offset Lcase3
        mov Ljmptab[4*4], offset Lcase4
        mov Ljmptab[5*4], offset Lcase5
        mov Ljmptab[6*4], offset Lcase6
        mov Ljmptab[7*4], offset Lcase7

initialized:

        mov edx, dword ptr[4+12+esp]
        mov ecx, dword ptr[4+4+esp]
        xor eax, eax
        mov ebx, dword ptr[4+8+esp]
        mov al, byte ptr[17+edx]
        cmp al, 8
        jae Lerror
        fld dword ptr[0+edx]
        fld st(0)
        jmp dword ptr[Ljmptab+eax*4]
Lcase0:
        fmul dword ptr[ebx]
        fld dword ptr[0+4+edx]
        fxch st(2)
        fmul dword ptr[ecx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[4+ebx]
        fld dword ptr[0+8+edx]
        fxch st(2)
        fmul dword ptr[4+ecx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[8+ebx]
        fxch st(5)
        faddp st(3), st(0)
        fmul dword ptr[8+ecx]
        fxch st(1)
        faddp st(3), st(0)
        fxch st(3)
        faddp st(2), st(0)
        jmp LSetSides
Lcase1:
        fmul dword ptr[ecx]
        fld dword ptr[0+4+edx]
        fxch st(2)
        fmul dword ptr[ebx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[4+ebx]
        fld dword ptr[0+8+edx]
        fxch st(2)
        fmul dword ptr[4+ecx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[8+ebx]
        fxch st(5)
        faddp st(3), st(0)
        fmul dword ptr[8+ecx]
        fxch st(1)
        faddp st(3), st(0)
        fxch st(3)
        faddp st(2), st(0)
        jmp LSetSides
Lcase2:
        fmul dword ptr[ebx]
        fld dword ptr[0+4+edx]
        fxch st(2)
        fmul dword ptr[ecx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[4+ecx]
        fld dword ptr[0+8+edx]
        fxch st(2)
        fmul dword ptr[4+ebx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[8+ebx]
        fxch st(5)
        faddp st(3), st(0)
        fmul dword ptr[8+ecx]
        fxch st(1)
        faddp st(3), st(0)
        fxch st(3)
        faddp st(2), st(0)
        jmp LSetSides
Lcase3:
        fmul dword ptr[ecx]
        fld dword ptr[0+4+edx]
        fxch st(2)
        fmul dword ptr[ebx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[4+ecx]
        fld dword ptr[0+8+edx]
        fxch st(2)
        fmul dword ptr[4+ebx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[8+ebx]
        fxch st(5)
        faddp st(3), st(0)
        fmul dword ptr[8+ecx]
        fxch st(1)
        faddp st(3), st(0)
        fxch st(3)
        faddp st(2), st(0)
        jmp LSetSides
Lcase4:
        fmul dword ptr[ebx]
        fld dword ptr[0+4+edx]
        fxch st(2)
        fmul dword ptr[ecx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[4+ebx]
        fld dword ptr[0+8+edx]
        fxch st(2)
        fmul dword ptr[4+ecx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[8+ecx]
        fxch st(5)
        faddp st(3), st(0)
        fmul dword ptr[8+ebx]
        fxch st(1)
        faddp st(3), st(0)
        fxch st(3)
        faddp st(2), st(0)
        jmp LSetSides
Lcase5:
        fmul dword ptr[ecx]
        fld dword ptr[0+4+edx]
        fxch st(2)
        fmul dword ptr[ebx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[4+ebx]
        fld dword ptr[0+8+edx]
        fxch st(2)
        fmul dword ptr[4+ecx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[8+ecx]
        fxch st(5)
        faddp st(3), st(0)
        fmul dword ptr[8+ebx]
        fxch st(1)
        faddp st(3), st(0)
        fxch st(3)
        faddp st(2), st(0)
        jmp LSetSides
Lcase6:
        fmul dword ptr[ebx]
        fld dword ptr[0+4+edx]
        fxch st(2)
        fmul dword ptr[ecx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[4+ecx]
        fld dword ptr[0+8+edx]
        fxch st(2)
        fmul dword ptr[4+ebx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[8+ecx]
        fxch st(5)
        faddp st(3), st(0)
        fmul dword ptr[8+ebx]
        fxch st(1)
        faddp st(3), st(0)
        fxch st(3)
        faddp st(2), st(0)
        jmp LSetSides
Lcase7:
        fmul dword ptr[ecx]
        fld dword ptr[0+4+edx]
        fxch st(2)
        fmul dword ptr[ebx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[4+ecx]
        fld dword ptr[0+8+edx]
        fxch st(2)
        fmul dword ptr[4+ebx]
        fxch st(2)
        fld st(0)
        fmul dword ptr[8+ecx]
        fxch st(5)
        faddp st(3), st(0)
        fmul dword ptr[8+ebx]
        fxch st(1)
        faddp st(3), st(0)
        fxch st(3)
        faddp st(2), st(0)
LSetSides:
        faddp st(2), st(0)
        fcomp dword ptr[12+edx]
        xor ecx, ecx
        fnstsw ax
        fcomp dword ptr[12+edx]
        and ah, 1
        xor ah, 1
        add cl, ah
        fnstsw ax
        and ah, 1
        add ah, ah
        add cl, ah
        pop ebx
        mov eax, ecx
        ret
Lerror:
        int 3
    }

    AssertMsg( "BoxOnPlaneSide: invalid signbits for plane" );
}

/* PointInArc  0x00473cb0 */
int PointInArc( const vec3_t pos, float arcRadius, const vec3_t arcOrigin,
                float halfThickness, float yawMax, float yawMin, float halfHeight )
{
    vec3_t delta;
    float  radius;
    float  radialDist;
    float  yaw;
    float  normalized;

    Assert( pos );
    Assert( arcOrigin );

    Vec3Sub( pos, arcOrigin, delta );

    radius     = Vec2Normalize( delta );
    radialDist = radius - arcRadius;

    if ( radialDist * radialDist <= halfThickness * halfThickness &&
         pos[2] >= arcOrigin[2] - halfHeight &&
         pos[2] <= arcOrigin[2] + halfHeight )
    {
        yaw        = vectoyaw( delta );
        normalized = AngleNormalize360( yaw );

        if ( yawMin <= yawMax )
        {
            if ( normalized < yawMin || normalized > yawMax )
                return 1;
        }
        else
        {
            if ( normalized < yawMin && normalized > yawMax )
                return 1;
        }
    }

    return 0;
}

/* BoundsDistSqExceeds  0x00473e00 */
bool BoundsDistSqExceeds( const vec3_t mins, const vec3_t maxs, const vec3_t point, float distSq )
{
    vec3_t dMin;
    vec3_t dMax;
    float  dist;
    float  term;
    int    i;

    Vec3Sub( mins, point, dMin );
    Vec3Sub( maxs, point, dMax );

    dist = 0.0f;

    for ( i = 0; i < 3; i++ )
    {
        if ( dMin[i] * dMax[i] > 0.0f )
        {
            term = dMin[i] * dMin[i];

            if ( dMax[i] * dMax[i] < term )
                term = dMax[i] * dMax[i];

            dist = dist + term;
        }
    }

    return dist > distSq;
}

/* Q_rint  0x00473ee0 */
float Q_rint( float value )
{
    return floor( ( float )( value + 0.5f ) );
}

/* Vec3MaxNormalize  0x00473f10 */
float Vec3MaxNormalize( const vec3_t in, vec3_t out )
{
    float max;

    max = in[0];

    if ( max < in[1] )
        max = in[1];

    if ( max < in[2] )
        max = in[2];

    if ( max == 0.0f )
    {
        out[0] = 1.0f;
        out[1] = 1.0f;
        out[2] = 1.0f;
        return 0.0f;
    }

    Vec3Scale( in, 1.0f / max, out );
    return max;
}

/* RotatePoint  0x00473fb0 */
void RotatePoint( const vec3_t point, const vec3_t angles, vec3_t out )
{
    int    rotLookup[3][2];
    vec3_t input;
    vec3_t result;
    double cosVal;
    double sinVal;
    int    i;
    int    ax1;
    int    ax2;

    Vec3Copy( point, input );
    Vec3Copy( input, result );

    rotLookup[0][0] = 1;
    rotLookup[0][1] = 2;
    rotLookup[1][0] = 2;
    rotLookup[1][1] = 0;
    rotLookup[2][0] = 0;
    rotLookup[2][1] = 1;

    for ( i = 0; i < 3; i++ )
    {
        if ( angles[i] != 0.0f )
        {
            SinCos( ( angles[i] * PI ) / 180.0, &sinVal, &cosVal );

            ax1 = rotLookup[i][0];
            ax2 = rotLookup[i][1];

            result[ax1] = ( float )( input[ax1] * cosVal - input[ax2] * sinVal );
            result[ax2] = ( float )( input[ax1] * sinVal + input[ax2] * cosVal );
        }

        Vec3Copy( result, input );
    }

    Vec3Copy( result, out );
}

/* RotatePointAboutOrigin  0x004740f0 */
void RotatePointAboutOrigin( const vec3_t point, const vec3_t angles, const vec3_t origin, vec3_t out )
{
    vec3_t delta;
    vec3_t rotated;

    Vec3Sub( point, origin, delta );
    RotatePoint( delta, angles, rotated );
    Vec3Add( rotated, origin, out );
}

/* SphericalToVec3  0x00474140 */
void SphericalToVec3( vec3_t out, float radius, float angle )
{
    float sinYaw, cosYaw;
    float sinPitch, cosPitch;

    SinCos( angle, &sinYaw, &cosYaw );
    SinCos( angle, &sinPitch, &cosPitch );

    out[0] = radius * cosYaw * cosPitch;
    out[1] = radius * sinYaw * cosPitch;
    out[2] = radius * sinPitch;
}

/* GetPlanePitchAtYaw  0x004741b0 */
float GetPlanePitchAtYaw( float yaw, const vec3_t normal )
{
    vec3_t forward;
    float  slope;

    Assert( normal[0] || normal[1] || normal[2] );

    YawVectors( yaw, forward, NULL );

    if ( normal[2] == 0.0f )
        return 270.0f;

    slope = ( normal[0] * forward[0] + normal[1] * forward[1] ) / normal[2];

    return ( float )atan( slope ) * 180.0 / PI;
}

/* ProjectAnglesOnPlane  0x00474280 */
void ProjectAnglesOnPlane( const vec3_t angles, const vec3_t normal, vec3_t outAngles )
{
    vec3_t forward;
    vec3_t projected;

    Assert( normal[0] || normal[1] || normal[2] );

    AngleVectors( angles, forward, NULL, NULL );
    ProjectPointOnPlane( forward, normal, projected );
    vectoangles( projected, outAngles );
}

/* Rand_SetSeed  0x00474320 */
void Rand_SetSeed( int seed )
{
    g_randSeed = seed;
}

/* flrand  0x00474330 */
float flrand( float min, float max )
{
    float sample;

    g_randSeed = g_randSeed * FLRAND_MULTIPLIER + FLRAND_ADDEND;
    sample     = ( float )( ( unsigned int )g_randSeed >> 17 );

    return ( float )( ( max - min ) * sample / RAND_SCALE + min );
}

/* irand  0x00474380 */
int irand( int min, int max )
{
    g_randSeed = g_randSeed * FLRAND_MULTIPLIER + FLRAND_ADDEND;

    return min + ( int )( ( ( __int64 )( ( unsigned int )g_randSeed >> 17 ) *
                            ( __int64 )( max - min ) ) >> 15 );
}

/* TransformPointByAnimMat  0x00474400 */
void TransformPointByAnimMat( const vec3_t point, const DObjAnimMat *mat, vec3_t out )
{
    DObjAnimMat_TransformPoint( point, mat, out );
}

/* AxisToQuat  0x00474420 */
void AxisToQuat( const float axis[3][3], vec4_t quat )
{
    vec4_t test[4];
    float  testSizeSq;
    int    index;

    test[0][0] = axis[1][2] - axis[2][1];
    test[0][1] = axis[2][0] - axis[0][2];
    test[0][2] = axis[0][1] - axis[1][0];
    test[0][3] = axis[0][0] + axis[1][1] + axis[2][2] + 1.0f;

    testSizeSq = Vec4LengthSq( test[0] );

    if ( testSizeSq < 1.0f )
    {
        test[1][0] = axis[2][0] + axis[0][2];
        test[1][1] = axis[2][1] + axis[1][2];
        test[1][2] = ( axis[2][2] - axis[1][1] ) - axis[0][0] + 1.0f;
        test[1][3] = test[0][2];

        testSizeSq = Vec4LengthSq( test[1] );

        if ( testSizeSq < 1.0f )
        {
            test[2][0] = ( axis[0][0] - axis[1][1] ) - axis[2][2] + 1.0f;
            test[2][1] = axis[1][0] + axis[0][1];
            test[2][2] = test[1][0];
            test[2][3] = test[0][0];

            testSizeSq = Vec4LengthSq( test[2] );

            if ( testSizeSq < 1.0f )
            {
                test[3][0] = test[2][1];
                test[3][1] = ( axis[1][1] - axis[0][0] ) - axis[2][2] + 1.0f;
                test[3][2] = test[1][1];
                test[3][3] = test[0][1];

                testSizeSq = Vec4LengthSq( test[3] );

                Assertx( testSizeSq >= 1.0f, "(testSizeSq) = %g", testSizeSq );

                index = 3;
            }
            else
            {
                index = 2;
            }
        }
        else
        {
            index = 1;
        }
    }
    else
    {
        index = 0;
    }

    Assert( testSizeSq );

    Vec4Scale( test[index], 1.0f / sqrtf( testSizeSq ), quat );
}

/* QuatLerp  0x00474650 */
void QuatLerp( const vec4_t from, const vec4_t to, float t, vec4_t out )
{
    if ( Vec4Dot( from, to ) < 0.0f )
    {
        Vec4Negate( to, out );
        Vec4Lerp( from, out, t, out );
    }
    else
    {
        Vec4Lerp( from, to, t, out );
    }
}

/* SinCosDegrees  0x004746d0 */
void SinCosDegrees( float degrees, float *sinOut, float *cosOut )
{
    Assert( sinOut );
    Assert( cosOut );

    if ( degrees < 0.0f )
        degrees = degrees + 360.0f;

    if ( degrees == 0.0f )
    {
        *cosOut = 1.0f;
        *sinOut = 0.0f;
    }
    else if ( degrees == 90.0f )
    {
        *cosOut = 0.0f;
        *sinOut = 1.0f;
    }
    else if ( degrees == 180.0f )
    {
        *cosOut = -1.0f;
        *sinOut = 0.0f;
    }
    else if ( degrees == 270.0f )
    {
        *cosOut = 0.0f;
        *sinOut = -1.0f;
    }
    else
    {
        SinCos( degrees * DEG2RAD, sinOut, cosOut );
    }
}

/* SnapToIntegralPowerOf2  0x004747f0 */
float SnapToIntegralPowerOf2( float value, int tolerance, byte powerBits )
{
    union
    {
        float f;
        int   i;
    } val;

    int power;
    int mask;
    int fraction;

    power    = 1 << powerBits;
    mask     = power - 1;
    val.f    = value;
    fraction = val.i & mask;

    if ( fraction > tolerance )
    {
        if ( power - fraction <= tolerance )
            val.i = ( power - fraction ) + val.i;
    }
    else
    {
        val.i = val.i - fraction;
    }

    return val.f;
}

/* SnapToGrid  0x00474850 */
float SnapToGrid( float value, float granularity, float epsilon )
{
    float scaled;
    float snapped;
    float diff;
    int   rounded;

    Assertx( granularity > 0, "(granularity) = %g", granularity );
    Assertx( epsilon > 0, "(epsilon) = %g", epsilon );
    Assertx( epsilon < 0.5f / granularity, "%g, %g", epsilon, granularity );

    value   = SnapToIntegralPowerOf2( value, SNAP_FLOAT_ULPS, SNAP_FLOAT_BITS );
    scaled  = value * granularity;
    rounded = RoundFloatToInt( scaled );
    snapped = ( float )rounded / granularity;
    diff    = I_fabs( snapped - value );

    if ( diff > epsilon )
        return value;

    return snapped;
}

/* BoxInCone  0x00474990 */
int BoxInCone( const vec3_t boxCenter, const vec3_t coneDir, float cosHalfFov,
               const vec3_t coneOrigin, const vec3_t boxHalfSize )
{
    vec3_t delta;
    vec3_t corner;
    vec3_t perp;
    vec3_t edge;
    float  dot;
    float  perpLenSq;
    float  sinSq;
    float  cosSq;
    float  scale;
    float  dist;

    Assert( cosHalfFov >= 0.0f );

    Vec3Sub( coneOrigin, boxCenter, delta );

    corner[0] = delta[0] - I_fsign( coneDir[0] ) * boxHalfSize[0];
    corner[1] = delta[1] - I_fsign( coneDir[1] ) * boxHalfSize[1];
    corner[2] = delta[2] - I_fsign( coneDir[2] ) * boxHalfSize[2];

    dot = Vec3Dot( corner, coneDir );

    if ( dot < 0.0f )
    {
        Vec3Mad( corner, -dot, coneDir, perp );
        perpLenSq = Vec3LengthSq( perp );

        cosSq = cosHalfFov * cosHalfFov;
        sinSq = 1.0f - cosSq;

        if ( perpLenSq * cosSq > dot * dot * sinSq )
        {
            scale = cosHalfFov / I_sqrt( sinSq * perpLenSq );
            Vec3Mad( coneDir, scale, perp, edge );

            dist = Vec3Dot( edge, delta );
            dist = dist - I_fabs( boxHalfSize[0] * edge[0] );
            dist = dist - I_fabs( boxHalfSize[1] * edge[1] );
            dist = dist - I_fabs( boxHalfSize[2] * edge[2] );

            return dist >= 0.0f;
        }

        return 0;
    }

    return 1;
}

/* SphereOverlapsBox  0x00474b90 */
int SphereOverlapsBox( const vec3_t sphereOrigin, float radius,
                       const vec3_t boxCenter, const vec3_t boxHalfSize )
{
    vec3_t d;

    d[0] = I_fmax( I_fabs( sphereOrigin[0] - boxCenter[0] ) - boxHalfSize[0], 0.0f );
    d[1] = I_fmax( I_fabs( sphereOrigin[1] - boxCenter[1] ) - boxHalfSize[1], 0.0f );
    d[2] = I_fmax( I_fabs( sphereOrigin[2] - boxCenter[2] ) - boxHalfSize[2], 0.0f );

    return Vec3LengthSq( d ) <= radius * radius;
}

/* BoxInConeRange  0x00474c80 */
int BoxInConeRange( const vec3_t boxCenter, const vec3_t coneDir, float cosHalfFov,
                    float range, const vec3_t coneOrigin, const vec3_t boxHalfSize )
{
    vec3_t delta;
    vec3_t clamped;
    vec3_t corner;
    vec3_t perp;
    vec3_t edge;
    float  dot;
    float  perpLenSq;
    float  sinSq;
    float  cosSq;
    float  scale;
    float  dist;

    Assert( cosHalfFov >= 0.0f );

    Vec3Sub( coneOrigin, boxCenter, delta );

    clamped[0] = I_fmax( I_fabs( delta[0] ) - boxHalfSize[0], 0.0f );
    clamped[1] = I_fmax( I_fabs( delta[1] ) - boxHalfSize[1], 0.0f );
    clamped[2] = I_fmax( I_fabs( delta[2] ) - boxHalfSize[2], 0.0f );

    if ( Vec3LengthSq( clamped ) <= range * range )
    {
        corner[0] = delta[0] - I_fsign( coneDir[0] ) * boxHalfSize[0];
        corner[1] = delta[1] - I_fsign( coneDir[1] ) * boxHalfSize[1];
        corner[2] = delta[2] - I_fsign( coneDir[2] ) * boxHalfSize[2];

        dot = Vec3Dot( corner, coneDir );

        if ( dot < 0.0f )
        {
            Vec3Mad( corner, -dot, coneDir, perp );
            perpLenSq = Vec3LengthSq( perp );

            cosSq = cosHalfFov * cosHalfFov;
            sinSq = 1.0f - cosSq;

            if ( perpLenSq * cosSq > dot * dot * sinSq )
            {
                scale = cosHalfFov / I_sqrt( sinSq * perpLenSq );
                Vec3Mad( coneDir, scale, perp, edge );

                dist = Vec3Dot( edge, delta );
                dist = dist - I_fabs( boxHalfSize[0] * edge[0] );
                dist = dist - I_fabs( boxHalfSize[1] * edge[1] );
                dist = dist - I_fabs( boxHalfSize[2] * edge[2] );

                return dist >= 0.0f;
            }

            return 0;
        }
    }

    return 1;
}

/* PointInConeCap  0x00474f30 */
int PointInConeCap( const vec3_t point, const vec3_t coneDir, float cosHalfFov,
                    const vec3_t coneOrigin, float capDist )
{
    vec3_t delta;
    vec3_t perp;
    float  dot;
    float  perpLenSq;
    float  sinSq;
    float  reach;

    Assert( cosHalfFov >= 0.0f );

    Vec3Sub( coneOrigin, point, delta );

    dot = Vec3Dot( delta, coneDir );

    if ( dot < capDist )
    {
        Vec3Mad( delta, -dot, coneDir, perp );
        perpLenSq = Vec3LengthSq( perp );

        sinSq = 1.0f - cosHalfFov * cosHalfFov;
        reach = I_sqrt( sinSq ) * dot - capDist;

        perpLenSq = perpLenSq * cosHalfFov * cosHalfFov;
        reach     = reach * reach;

        if ( reach > perpLenSq )
            return 0;
    }

    return 1;
}

/* Vec3DistanceToCone  0x00475020 */
float Vec3DistanceToCone( const vec3_t point, const vec3_t coneDir, float cosHalfFov,
                          const vec3_t coneOrigin )
{
    vec3_t delta;
    vec3_t perp;
    float  dot;
    float  perpLen;
    float  sinHalfFov;
    float  dist;

    Assert( cosHalfFov >= 0.0f );

    Vec3Sub( coneOrigin, point, delta );

    dot = Vec3Dot( delta, coneDir );
    Vec3Mad( delta, -dot, coneDir, perp );
    perpLen = Vec3Length( perp );

    sinHalfFov = I_sqrt( 1.0f - cosHalfFov * cosHalfFov );

    if ( dot <= 0.0f || dot * cosHalfFov <= perpLen * sinHalfFov )
    {
        dist = perpLen * cosHalfFov + dot * sinHalfFov;
        return I_fmax( 0.0f, dist );
    }

    return Vec3Length( delta );
}

/* Vec3DistanceToConeVolume  0x00475130 */
float Vec3DistanceToConeVolume( const vec3_t point, const vec3_t coneDir, float cosHalfFov,
                                float range, const vec3_t coneOrigin )
{
    vec3_t delta;
    vec3_t closest;
    float  lenSq;
    float  along;
    float  perp;
    float  sinHalfFov;
    float  outward;
    float  side;
    float  t;
    float  s;
    float  w;

    Assert( cosHalfFov >= 0.0f );

    Vec3Sub( coneOrigin, point, delta );

    lenSq = Vec3LengthSq( delta );
    along = Vec3Dot( delta, coneDir );
    perp  = I_sqrt( lenSq - along * along );

    sinHalfFov = I_sqrt( 1.0f - cosHalfFov * cosHalfFov );
    outward    = perp * cosHalfFov + along * sinHalfFov;

    if ( along >= 0.0f || outward >= 0.0f )
    {
        side = perp * sinHalfFov - along * cosHalfFov;

        if ( side < 0.0f )
            return I_sqrt( lenSq );

        side = I_fmin( side, range );
        t    = ( side * sinHalfFov ) / perp;
        s    = 1.0f - t;
        w    = along * t + side * cosHalfFov;

        Vec3Combine( s, delta, w, coneDir, closest );
        return Vec3Length( closest );
    }

    if ( lenSq >= range * range )
        return I_sqrt( lenSq ) - range;

    return 0.0f;
}

/* Vec3ClosestPointOnConeVolume  0x004752f0 */
void Vec3ClosestPointOnConeVolume( const vec3_t point, const vec3_t coneDir, float cosHalfFov,
                                   float range, const vec3_t coneOrigin, vec3_t out )
{
    vec3_t delta;
    float  lenSq;
    float  along;
    float  perp;
    float  sinHalfFov;
    float  outward;
    float  side;
    float  t;
    float  s;
    float  w;

    Assert( cosHalfFov >= 0.0f );

    Vec3Sub( coneOrigin, point, delta );

    lenSq = Vec3LengthSq( delta );
    along = Vec3Dot( delta, coneDir );
    perp  = I_sqrt( lenSq - along * along );

    sinHalfFov = I_sqrt( 1.0f - cosHalfFov * cosHalfFov );
    outward    = perp * cosHalfFov + along * sinHalfFov;

    if ( along >= 0.0f || outward >= 0.0f )
    {
        side = perp * sinHalfFov - along * cosHalfFov;

        if ( side < 0.0f )
        {
            Vec3Copy( point, out );
            return;
        }

        side = I_fmin( side, range );
        t    = ( side * sinHalfFov ) / perp;
        s    = 1.0f - t;
        w    = along * t + side * cosHalfFov;

        Vec3MadCombine( point, s, delta, w, coneDir, out );
        return;
    }

    if ( lenSq >= range * range )
    {
        Vec3Mad( point, range / I_sqrt( lenSq ), delta, out );
        return;
    }

    Vec3Copy( coneOrigin, out );
}


/* Vec3DominantAxis  0x00438300 */
int Vec3DominantAxis( const vec3_t v )
{
    float square[3];
    int   axis;

    Assert( v );

    square[0] = v[0] * v[0];
    square[1] = v[1] * v[1];
    square[2] = v[2] * v[2];

    axis = square[0] >= square[1] ? 0 : 1;

    return square[axis] >= square[2] ? axis : 2;
}
