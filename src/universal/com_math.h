/* Original: ..\src\universal\com_math.h */

#ifndef COM_MATH_H
#define COM_MATH_H


#include "assertive.h"
#include "q_shared.h"


#define PI                  3.1415927f

#define DEG2RAD             0.017453292f

#define HALF_DEG2RAD        0.008726646f

#define RAD2DEG             ( 180.0f / PI )

#define TWO_PI              6.2831855f

#define MAX_WORLD_COORD     131072.0f
#define MIN_WORLD_COORD     -131072.0f

#define RAND_SCALE          32768.0

#define FLRAND_MULTIPLIER   0x343fd
#define FLRAND_ADDEND       0x269ec3

#define RAND_MULTIPLIER     1103515245
#define RAND_ADDEND         12345

#define SPREAD_COS_STEP     -0.7373689f
#define SPREAD_SIN_STEP     -0.6754903f

#define SPREAD_COS_START    0.94551855f
#define SPREAD_SIN_START    0.32556814f

#define INFINITE_FAR_SCALE  0.9995117f

#define DIFFTRACK_EPSILON   0.001f

#define SLERP_LINEAR_COS    0.95f

#define SNAP_FLOAT_BITS     12
#define SNAP_FLOAT_ULPS     4
#define IS_NAN( x )         Float_IsNanOrInf( x )


/* Clamp  0x0040df00 */
float Clamp( float value, float min, float max );

/* I_fsel  0x00405400 */
float I_fsel( float comparand, float valGE, float valLT );

/* I_fmin  0x004053d0 */
float I_fmin( float a, float b );

/* I_fmax  0x0040a2f0 */
float I_fmax( float a, float b );

/* I_fclamp  0x00435ef0 */
float I_fclamp( float value, float min, float max );

/* I_fsign  0x00475c10 */
float I_fsign( float x );

/* I_fabs  0x00405310 */
float I_fabs( float x );

/* I_sqrt  0x00407700 */
float I_sqrt( float x );

/* I_min  0x004122f0 */
int I_min( int a, int b );

/* I_max  0x004122c0 */
int I_max( int a, int b );

/* Float_GetBits  0x004758d0 */
int Float_GetBits( const float *f );

/* Float_IsNanOrInf  0x0044c140 */
bool Float_IsNanOrInf( float f );

#ifdef _WIN64
#include <emmintrin.h>

/* Rounds to nearest even, like fistp does in the default x87 rounding mode */
inline int Fistp( double value )
{
    return _mm_cvtsd_si32( _mm_set_sd( value ) );
}
#else
inline int Fistp( double value )
{
    int result;

    __asm
    {
        fld   value
        fistp result
    }

    return result;
}
#endif

/* RoundFloatToInt  0x00416540 */
#define FISTP_BIAS          9.313225746154785e-10

int RoundFloatToInt( float value );

/* FloorFloatToInt  0x0042ac10 */
#define FISTP_HALF_BIAS     0.4999999990686774

int FloorFloatToInt( float value );

/* RoundFloatToIntHalf  0x0044c300 */
int RoundFloatToIntHalf( float value );

/* SinCos  0x00412890 */
void SinCos( float angle, float *sinOut, float *cosOut );

/* SinCos  0x004758e0 */
void SinCos( double angle, double *sinOut, double *cosOut );

/* AngleNormalize180  0x00475500 */
float AngleNormalize180( float angle );

/* AngleSubtract  0x004758b0 */
float AngleSubtract( float a1, float a2 );


int VecNCompareEpsilon( const float *v0, const float *v1, float epsilon, int count );

float Vec3ConeDistance( const vec3_t apex, const vec3_t dir, float cosHalfFov,
                        const vec3_t point );                              /* 0x0043f010 */

float Vec3MaxElement( const vec3_t v );                                    /* 0x004385e0 */


float   random( void );                                                 /* 0x0046cac0 */
float   crandom( void );                                                /* 0x0046caf0 */
void    RandGaussianPair( float *f0, float *f1 );                       /* 0x0046cb10 */
int     RandInt( int *seed );                                           /* 0x0046cbe0 */

void    SpreadPointInCircle( float radiusSq, float angleFrac, vec2_t out );        /* 0x0046cc20 */
void    SpreadPointOnSphere( float heightFrac, float angleFrac, vec3_t out );      /* 0x0046cc80 */
void    SpreadPointOnHemisphere( float height, float angleFrac, vec3_t out );      /* 0x0046cd00 */
void    SpreadPointsInCircle( unsigned int count, vec2_t *points, unsigned int stride );      /* 0x0046cd80 */
void    SpreadPointsOnHemisphere( unsigned int count, vec3_t *points, unsigned int stride );  /* 0x0046cea0 */
void    SpreadPointsOnSphere( unsigned int count, vec3_t *points, unsigned int stride );      /* 0x0046cfe0 */

float   DiffTrack( float target, float current, float rate, float deltaTime );            /* 0x0046d120 */
float   AngleDiffTrack( float target, float current, float rate, float deltaTime );       /* 0x0046d1c0 */
float   DiffTrackSimple( float target, float current, float rate, float deltaTime );      /* 0x0046d240 */
float   AngleDiffTrackSimple( float target, float current, float rate, float deltaTime ); /* 0x0046d2c0 */

float   EvalKnots( int knotCount, const vec2_t *knots, float fraction );  /* 0x0046d340 */
int     Log2ForBitCount( int value );                                     /* 0x0046d610 */
float   ACos( float x );                                                  /* 0x0046d640 */
char    ClampChar( int i );                                               /* 0x0046d690 */
short   ClampShort( int i );                                              /* 0x0046d6b0 */

byte    DirToByte( const vec3_t dir );                                    /* 0x0046d6e0 */
void    ByteToDir( int b, vec3_t dir );                                   /* 0x0046d760 */

float   Vec3DistanceSqToRay( const vec3_t point, const vec3_t start,
                             const vec3_t dir, float length );            /* 0x0046d810 */
float   Vec2Distance( const vec2_t v0, const vec2_t v1 );                 /* 0x0046d8a0 */
float   Vec2DistanceSq( const vec2_t v0, const vec2_t v1 );               /* 0x0046d8d0 */
int     VecLargestAxis( const vec3_t dir );                               /* 0x0046d910 */
void    GetProjectionAxes( const vec3_t normal, int *axisU, int *axisV );  /* 0x0046d9a0 */
int     Vec3DominantAxis( const vec3_t v );                               /* 0x00438300 */

float   Vec2Normalize( vec2_t v );                                        /* 0x0046dab0 */
float   Vec3NormalizeTo( const vec3_t in, vec3_t out );                   /* 0x0046db40 */

/* AnglesToAxis  0x00412690 */
void    AnglesToAxis( const vec3_t angles, vec3_t axis[3] );
float   Vec2NormalizeTo( const vec2_t in, vec2_t out );                   /* 0x0046dbe0 */
float   Vec3MaxComponent( const vec3_t v );                               /* 0x0046dc70 */

void    MatrixTransformVector33( const vec3_t in, const float mat[3][3], vec3_t out );           /* 0x0046dce0 */
void    MatrixTransposeTransformVector33( const vec3_t in, const float mat[3][3], vec3_t out );  /* 0x0046dda0 */

void    RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees ); /* 0x0046de60 */
void    AxisFromForwardAndRoll( float axis[3][3], float roll );           /* 0x0046e160 */
void    MakeNormalVectors( const vec3_t forward, vec3_t up, vec3_t right );   /* 0x0046e1e0 */
void    MakeNormalVectors2( const vec3_t forward, vec3_t up, vec3_t right );  /* 0x0046e210 */

float   vectoyaw( const vec3_t vec );                                     /* 0x0046e240 */
float   vectoyaw180( const vec3_t vec );                                  /* 0x0046e2d0 */
float   vectopitch( const vec3_t vec );                                   /* 0x0046e3a0 */
float   vectopitch_no360( const vec3_t vec );                             /* 0x0046e480 */
void    vectoangles( const vec3_t vec, vec3_t angles );                   /* 0x0046e540 */
void    QuatToAngles( const vec4_t quat, vec3_t angles );                 /* 0x0046e690 */
void    vectoangles_alt( const vec3_t vec, vec3_t angles );               /* 0x0046e6c0 */
void    YawVectors( float yaw, vec3_t forward, vec3_t right );            /* 0x0046e7c0 */
void    YawVectors2D( float yaw, vec2_t forward, vec2_t right );          /* 0x0046e830 */
void    PerpendicularVector( const vec3_t src, vec3_t dst );              /* 0x0046e890 */
void    TriangleNormal( const vec3_t p0, const vec3_t p1, const vec3_t p2, vec3_t normal ); /* 0x0046e9c0 */

void    ProjectPointOntoVector( const vec3_t point, const vec3_t vStart,
                                const vec3_t vEnd, vec3_t out );          /* 0x0046ea30 */
float   PointLineDistSq( const vec3_t point, const vec3_t start, const vec3_t end );   /* 0x0046ead0 */
float   Vec3DistanceSqToBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs ); /* 0x0046eb90 */
float   PointLineDistSq2D( const vec2_t point, const vec2_t start, const vec2_t end ); /* 0x0046ec20 */
float   PointLineDirDistSq( const vec3_t point, const vec3_t linePoint, const vec3_t lineDir ); /* 0x0046ece0 */
void    PointOnLineDir( const vec3_t point, const vec3_t linePoint,
                        const vec3_t lineDir, vec3_t out );               /* 0x0046ed70 */
void    ClosestPointsBetweenLines( const vec3_t p1, const vec3_t dir1,
                                   const vec3_t p2, const vec3_t dir2,
                                   float *outT1, float *outT2 );          /* 0x0046edf0 */

void    MatrixIdentity33( float mtx[3][3] );                              /* 0x0046ef50 */
void    MatrixIdentity44( float mtx[4][4] );                              /* 0x0046efb0 */
void    MatrixSetTransform44( float mtx[4][4], const vec3_t origin,
                              const float axis[3][3], float scale );      /* 0x0046eff0 */
void    MatrixMultiply33( const float in1[3][3], const float in2[3][3], float out[3][3] ); /* 0x0046f0c0 */
void    MatrixPreMultiply33( const float in1[3][3], float mtx[3][3] );    /* 0x0046f260 */
void    MatrixMultiply34( const float in1[3][4], const float in2[3][4], float out[3][4] ); /* 0x0046f420 */
void    MatrixMultiply43( const float in1[4][3], const float in2[4][3], float out[4][3] ); /* 0x0046f6b0 */
void    MatrixMultiply44( const float in1[4][4], const float in2[4][4], float out[4][4] ); /* 0x0046f940 */
void    MatrixTranspose33( const float in[3][3], float out[3][3] );       /* 0x0046fd50 */
void    MatrixTranspose44( const float in[4][4], float out[4][4] );       /* 0x0046fdf0 */
void    MatrixInverse33( const float in[3][3], float out[3][3] );         /* 0x0046fee0 */
void    MatrixInverseOrthonormal43( const float in[4][3], float out[4][3] ); /* 0x004700f0 */
void    MatrixInverse44( const float mat[4][4], float dst[4][4] );        /* 0x00470140 */
void    MatrixTransformVector44( const vec4_t vec, const float mat[4][4], vec4_t out );   /* 0x00470730 */
void    MatrixInverseTransformVector43( const vec3_t in1, const float in2[3][3], vec3_t out ); /* 0x00470850 */
void    MatrixTransformPoint43( const vec3_t in1, const float in2[4][3], vec3_t out );    /* 0x00470910 */
void    MatrixInverseTransformPoint43( const vec3_t in1, const float in2[4][3], vec3_t out ); /* 0x004709e0 */
void    MatrixTransformPointInPlace43( vec3_t inout, const float mat[4][3] );             /* 0x00470aa0 */
void    Vec2Rotate( vec2_t v, float degrees );                            /* 0x00470b50 */

void    QuatConjugate( const vec4_t quat, vec4_t out );                   /* 0x00470bc0 */
void    QuatToAxis( const vec4_t quat, float axis[3][3] );                /* 0x00470c00 */
void    UnitQuatToAxis( const vec4_t quat, float axis[3][3] );            /* 0x00470d90 */
void    UnitQuatToForward( const vec4_t quat, vec3_t forward );           /* 0x00470f20 */
void    QuatSlerp( const vec4_t from, const vec4_t to, float t, vec4_t out ); /* 0x00471010 */
float   QuatRunningAveragePortion( const vec4_t quat );                   /* 0x004711d0 */
float   SinSqDegrees( float degrees );                                    /* 0x00471270 */
float   QuatDifferencePortion( const vec4_t q0, const vec4_t q1 );        /* 0x004712b0 */
float   QuatToAngleDeg( const vec2_t quat );                              /* 0x004712f0 */
void    AxisAngleToQuat( float degrees, const vec3_t axis, vec4_t quat ); /* 0x004713a0 */

void    MatrixRotationX( float mtx[3][3], float degrees );                /* 0x004713f0 */
void    MatrixRotationY( float mtx[3][3], float degrees );                /* 0x00471470 */
void    MatrixRotationZ( float mtx[3][3], float degrees );                /* 0x004714f0 */
void    MatrixPerspectiveProjection( float mtx[4][4], float tanHalfFovX, float tanHalfFovY,
                                     float zNear, float zFar );           /* 0x00471570 */
void    MatrixPerspectiveProjectionInfinite( float mtx[4][4], float tanHalfFovX,
                                             float tanHalfFovY, float zNear ); /* 0x00471670 */
void    MatrixOrthoProjection( float mtx[4][4], float width, float height, float depth ); /* 0x00471720 */
void    MatrixWorldToCamera( float mtx[4][4], const vec3_t origin, const float axis[3][3] ); /* 0x00471830 */

void    AnglesSubtract( const vec3_t a0, const vec3_t a1, vec3_t out );   /* 0x004719d0 */
float   AngleNormalize360( float angle );                                 /* 0x00471a40 */
float   AngleDelta( float a1, float a2 );                                 /* 0x00471aa0 */

float   RadiusFromBounds( const vec3_t mins, const vec3_t maxs );         /* 0x00471ac0 */
float   RadiusFromBounds2D( const vec2_t mins, const vec2_t maxs );       /* 0x00471ae0 */
float   RadiusFromBoundsSq( const vec3_t mins, const vec3_t maxs );       /* 0x00471b00 */
float   RadiusFromBounds2DSq( const vec2_t mins, const vec2_t maxs );     /* 0x00471b90 */
void    ExpandBoundsForOffset( vec3_t mins, vec3_t maxs, const vec3_t offset ); /* 0x00471c20 */
void    ExpandBoundsToCube( vec3_t mins, vec3_t maxs );                   /* 0x00471cd0 */
void    ShrinkBoundsToCube( vec3_t mins, vec3_t maxs );                   /* 0x00471de0 */
void    ClearBounds( vec3_t mins, vec3_t maxs );                          /* 0x00471f50 */
bool    BoundsAreCleared( const vec3_t mins, const vec3_t maxs );          /* 0x00471fb0 */
void    ClearBounds2D( vec2_t mins, vec2_t maxs );                        /* 0x00472030 */
void    AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs );     /* 0x00472080 */
void    AddPointToBounds2D( const vec2_t v, vec2_t mins, vec2_t maxs );   /* 0x00472150 */
int     PointInBounds( const vec3_t v, const vec3_t mins, const vec3_t maxs );   /* 0x004721e0 */
int     PointInBounds2D( const vec2_t v, const vec2_t mins, const vec2_t maxs ); /* 0x004722f0 */
int     BoundsOverlap( const vec3_t mins0, const vec3_t maxs0,
                       const vec3_t mins1, const vec3_t maxs1 );          /* 0x004723d0 */
int     BoundsOverlap2D( const vec2_t mins0, const vec2_t maxs0,
                         const vec2_t mins1, const vec2_t maxs1 );        /* 0x00472460 */
int     BoundsOverlapTolerance( const vec3_t mins0, const vec3_t maxs0,
                                const vec3_t mins1, const vec3_t maxs1, float tolerance ); /* 0x004724d0 */
int     BoundsOverlapTolerance2D( const vec2_t mins0, const vec2_t maxs0,
                                  const vec2_t mins1, const vec2_t maxs1, float tolerance ); /* 0x00472580 */
void    AddBoundsToBounds( const vec3_t srcMins, const vec3_t srcMaxs,
                           vec3_t dstMins, vec3_t dstMaxs );              /* 0x004725f0 */
void    AddBoundsToBounds2D( const vec2_t srcMins, const vec2_t srcMaxs,
                             vec2_t dstMins, vec2_t dstMaxs );            /* 0x004726c0 */
float   BoundsVolume( const vec3_t mins, const vec3_t maxs );             /* 0x00472750 */
void    MatrixTransformBounds( const vec3_t bounds[2], const vec3_t origin,
                               const float mat[3][3], vec3_t outBounds[2] ); /* 0x00472790 */

void    AxisClear( vec3_t axis[3] );                                      /* 0x00472940 */
void    AxisCopy( const vec3_t from[3], vec3_t to[3] );                   /* 0x00472990 */
void    AxisTranspose( const vec3_t in[3], vec3_t out[3] );               /* 0x004729e0 */
void    AxisTransformScalars( const vec3_t axis[3], float x, float y, float z, vec3_t out ); /* 0x00472a80 */
void    AxisTransformVec3( const vec3_t axis[3], const vec3_t in, vec3_t out ); /* 0x00472b00 */
void    YawToAxis( float yaw, vec3_t axis[3] );                           /* 0x00472b90 */
void    AxisToAngles( const vec3_t axis[3], vec3_t angles );              /* 0x00472bf0 */
void    AxisToAnglesAlt( const vec3_t axis[3], vec3_t angles );           /* 0x00472d10 */

bool    PlaneIntersection3( const vec4_t *planes[3], vec3_t outPoint );   /* 0x00472e30 */
void    SnapPlaneIntersection( const vec4_t *planes[3], vec3_t point,
                               float gridSize, float tolerance );         /* 0x004730f0 */
void    SnapPointToPlanes( const vec4_t *planeArray, int numPlanes, vec3_t point,
                           float gridSize, float tolerance );             /* 0x00473290 */
int     PointInPoly( const vec3_t *points, int numPoints, int axisU, int axisV,
                     const vec3_t point );                                /* 0x004734b0 */
int     PointInPolyEpsilon( const vec3_t *points, int numPoints, int axisU, int axisV,
                            const vec3_t point, float epsilon );          /* 0x00473590 */
int     PolyClipFraction( const vec3_t *points, int numPoints, int axisU, int axisV,
                          const vec3_t start, const vec3_t end, float *outFrac ); /* 0x004736a0 */
int     PlaneFromPoints( vec4_t plane, const vec3_t point0, const vec3_t point1,
                         const vec3_t point2 );                           /* 0x00473820 */
void    ProjectPointOnPlane( const vec3_t point, const vec3_t normal, vec3_t out ); /* 0x004739a0 */
int     BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, const struct cplane_s *p ); /* 0x00473a50 */

int     PointInArc( const vec3_t pos, float arcRadius, const vec3_t arcOrigin,
                    float halfThickness, float yawMax, float yawMin, float halfHeight ); /* 0x00473cb0 */
bool    BoundsDistSqExceeds( const vec3_t mins, const vec3_t maxs,
                             const vec3_t point, float distSq );          /* 0x00473e00 */

float   Q_rint( float value );                                            /* 0x00473ee0 */
float   Vec3MaxNormalize( const vec3_t in, vec3_t out );                  /* 0x00473f10 */
void    RotatePoint( const vec3_t point, const vec3_t angles, vec3_t out ); /* 0x00473fb0 */
void    RotatePointAboutOrigin( const vec3_t point, const vec3_t angles,
                                const vec3_t origin, vec3_t out );        /* 0x004740f0 */
void    SphericalToVec3( vec3_t out, float radius, float angle );         /* 0x00474140 */
float   GetPlanePitchAtYaw( float yaw, const vec3_t normal );             /* 0x004741b0 */
void    ProjectAnglesOnPlane( const vec3_t angles, const vec3_t normal, vec3_t outAngles ); /* 0x00474280 */

void    Rand_SetSeed( int seed );                                         /* 0x00474320 */
float   flrand( float min, float max );                                   /* 0x00474330 */
int     irand( int min, int max );                                        /* 0x00474380 */
struct  DObjAnimMat;
void    TransformPointByAnimMat( const vec3_t point, const struct DObjAnimMat *mat,
                                 vec3_t out );                            /* 0x00474400 */

void    AxisToQuat( const float axis[3][3], vec4_t quat );                /* 0x00474420 */
void    QuatLerp( const vec4_t from, const vec4_t to, float t, vec4_t out ); /* 0x00474650 */
void    SinCosDegrees( float degrees, float *sinOut, float *cosOut );     /* 0x004746d0 */

float   SnapToIntegralPowerOf2( float value, int tolerance, byte powerBits ); /* 0x004747f0 */
float   SnapToGrid( float value, float granularity, float epsilon );      /* 0x00474850 */

int     BoxInCone( const vec3_t boxCenter, const vec3_t coneDir, float cosHalfFov,
                   const vec3_t coneOrigin, const vec3_t boxHalfSize );   /* 0x00474990 */
int     SphereOverlapsBox( const vec3_t sphereOrigin, float radius,
                           const vec3_t boxCenter, const vec3_t boxHalfSize ); /* 0x00474b90 */
int     BoxInConeRange( const vec3_t boxCenter, const vec3_t coneDir, float cosHalfFov,
                        float range, const vec3_t coneOrigin, const vec3_t boxHalfSize ); /* 0x00474c80 */
int     PointInConeCap( const vec3_t point, const vec3_t coneDir, float cosHalfFov,
                        const vec3_t coneOrigin, float capDist );         /* 0x00474f30 */
float   Vec3DistanceToCone( const vec3_t point, const vec3_t coneDir, float cosHalfFov,
                            const vec3_t coneOrigin );                    /* 0x00475020 */
float   Vec3DistanceToConeVolume( const vec3_t point, const vec3_t coneDir, float cosHalfFov,
                                  float range, const vec3_t coneOrigin ); /* 0x00475130 */
void    Vec3ClosestPointOnConeVolume( const vec3_t point, const vec3_t coneDir, float cosHalfFov,
                                      float range, const vec3_t coneOrigin, vec3_t out ); /* 0x004752f0 */


/* MatrixTransformVector  0x00475580 */
void MatrixTransformVector( const vec3_t in1, const float in2[3][3], vec3_t out );

/* QuatMultiply  0x00475770 */
void QuatMultiply( const vec4_t in1, const vec4_t in2, vec4_t out );

#include "com_vector.h"

#endif
