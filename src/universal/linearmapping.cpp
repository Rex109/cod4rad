/* Original: c:\trees\cod3\cod3src\src\universal\linearmapping.cpp */

#include "cod4rad.h"
#include "linearmapping.h"
#include "com_math.h"
#include "assertive.h"

#include <math.h>


/* Vec3ScaleToMax  0x004177b0 */
float Vec3ScaleToMax( vec3_t v )
{
    float largest;
    float scale;

    largest = v[0];

    if ( v[1] > largest )
        largest = v[1];

    if ( v[2] > largest )
        largest = v[2];

    if ( largest == 0.0f )
        return largest;

    scale = 1.0f / largest;

    v[0] = v[0] * scale;
    v[1] = v[1] * scale;
    v[2] = v[2] * scale;

    return largest;
}

/* LU_Decompose  0x00416fe0 */
static qboolean LU_Decompose( double a[3][3], int index[3] )
{
    double scale[3];
    double big;
    double sum;
    double swap;
    int    i;
    int    j;
    int    k;
    int    largest = 0;

    for ( i = 0; i < 3; i++ )
    {
        big = 0.0;

        for ( j = 0; j < 3; j++ )
            if ( fabs( a[i][j] ) > big )
                big = fabs( a[i][j] );

        if ( big == 0.0 )
            return qfalse;

        scale[i] = 1.0 / big;
    }

    for ( j = 0; j < 3; j++ )
    {
        for ( i = 0; i < j; i++ )
        {
            sum = a[i][j];

            for ( k = 0; k < i; k++ )
                sum -= a[i][k] * a[k][j];

            a[i][j] = sum;
        }

        big = 0.0;

        for ( i = j; i < 3; i++ )
        {
            sum = a[i][j];

            for ( k = 0; k < j; k++ )
                sum -= a[i][k] * a[k][j];

            a[i][j] = sum;

            if ( scale[i] * fabs( sum ) > big )
            {
                big     = scale[i] * fabs( sum );
                largest = i;
            }
        }

        if ( a[largest][j] == 0.0 )
            return qfalse;

        if ( j != largest )
        {
            for ( k = 0; k < 3; k++ )
            {
                swap          = a[largest][k];
                a[largest][k] = a[j][k];
                a[j][k]       = swap;
            }

            scale[largest] = scale[j];
        }

        index[j] = largest;

        if ( j == 2 )
            break;

        swap = 1.0 / a[j][j];

        for ( i = j + 1; i < 3; i++ )
            a[i][j] *= swap;
    }

    if ( a[0][0] == 0.0 )
        return qfalse;

    if ( a[1][1] == 0.0 )
        return qfalse;

    if ( a[2][2] == 0.0 )
        return qfalse;

    return qtrue;
}

/* LU_BackSubstitute  0x00417340 */
static void LU_BackSubstitute( const double a[3][3], double b[3], const int index[3] )
{
    double sum;
    int    first = -1;
    int    i;
    int    j;

    for ( i = 0; i < 3; i++ )
    {
        int    pivot = index[i];

        sum      = b[pivot];
        b[pivot] = b[i];

        if ( first >= 0 )
        {
            for ( j = first; j <= i - 1; j++ )
                sum -= a[i][j] * b[j];
        }
        else if ( sum != 0.0 )
        {
            first = i;
        }

        b[i] = sum;
    }

    for ( i = 2; i >= 0; i-- )
    {
        sum = b[i];

        for ( j = i + 1; j < 3; j++ )
            sum -= a[i][j] * b[j];

        b[i] = sum / a[i][i];
    }
}

/* LU_Refine  0x004174f0 */
static void LU_Refine( const double lu[3][3], const int index[3],
                       const double a[3][3], const double rhs[3], double x[3] )
{
    double residual[3];
    int    i;

    for ( i = 0; i < 3; i++ )
        residual[i] = ( a[i][0] * x[0] - rhs[i] ) + a[i][1] * x[1] + a[i][2] * x[2];

    LU_BackSubstitute( lu, residual, index );

    x[0] -= residual[0];
    x[1] -= residual[1];
    x[2] -= residual[2];
}

/* LinearMapping_Evaluate  0x00417560 */
static void LinearMapping_Evaluate( const LinearMapping_t *mapping, float v0, float v1,
                                    float v2, int axis0, int axis1, int axis2,
                                    const int *index, const double lu[3][3], vec4_t out )
{
    double solution[3];
    double rhs[3];

    solution[0] = rhs[0] = v0;
    solution[1] = rhs[1] = v1;
    solution[2] = rhs[2] = v2;

    LU_BackSubstitute( lu, solution, index );
    LU_Refine( lu, index, mapping->matrix, rhs, solution );

    out[axis0] = ( float )solution[0];
    out[axis1] = ( float )solution[1];
    out[axis2] = 0.0f;
    out[3]     = ( float )solution[2];

    Assertx( !IS_NAN( out[0] ) && !IS_NAN( out[1] ) && !IS_NAN( out[2] )
             && !IS_NAN( out[3] ),
             "!IS_NAN((out)[0]) && !IS_NAN((out)[1]) && !IS_NAN((out)[2])"
             " && !IS_NAN((out)[3])" );
}

/* LinearMapping_Solve  0x00417650 */
qboolean LinearMapping_Solve( const vec3_t normal, const vec3_t p0, const vec3_t p1,
                              const vec3_t p2, LinearMapping_t *mapping )
{
    mapping->axis2 = Vec3DominantAxis( normal );

    mapping->axis1 = ~mapping->axis2 & 2;
    mapping->axis0 = ~mapping->axis2 & 1;

    mapping->matrix[0][0] = p0[mapping->axis0];
    mapping->matrix[0][1] = p0[mapping->axis1];
    mapping->matrix[0][2] = 1.0;

    mapping->matrix[1][0] = p1[mapping->axis0];
    mapping->matrix[1][1] = p1[mapping->axis1];
    mapping->matrix[1][2] = 1.0;

    mapping->matrix[2][0] = p2[mapping->axis0];
    mapping->matrix[2][1] = p2[mapping->axis1];
    mapping->matrix[2][2] = 1.0;

    memcpy( mapping->lu, mapping->matrix, sizeof( mapping->matrix ) );

    return LU_Decompose( mapping->lu, mapping->pivot ) != qfalse;
}

/* LinearMapping_Apply  0x004176e0 */
void LinearMapping_Apply( const LinearMapping_t *mapping, float v0, float v1, float v2,
                          vec4_t out )
{
    Assertx( mapping, "mapping" );

    LinearMapping_Evaluate( mapping, v0, v1, v2,
                            mapping->axis0, mapping->axis1, mapping->axis2,
                            mapping->pivot, mapping->lu, out );
}
