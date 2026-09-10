/* Original: ..\src\physics\ode\src\mass.cpp */

#ifndef ODE_MASS_H
#define ODE_MASS_H

#include "matrix.h"

struct dMass;
void dMassSetZero ( dMass *m );                                      /* 0x0048e660 */

struct dMass
{
    dReal    mass;      /* +0x00 */
    dVector4 c;         /* +0x04 */
    dMatrix3 I;         /* +0x14 */

    dMass()  { dMassSetZero( this ); }                   /* 0x0048f5f0 */
};

#define _I(i,j) I[(i)*4+(j)]

void dMassSetParameters ( dMass *m, dReal themass,
                          dReal cgx, dReal cgy, dReal cgz,
                          dReal I11, dReal I22, dReal I33,
                          dReal I12, dReal I13, dReal I23 );         /* 0x0048e6c0 */

void dMassSetSphere      ( dMass *m, dReal density, dReal radius );  /* 0x0048e920 */
void dMassSetSphereTotal ( dMass *m, dReal total_mass, dReal radius );
                                                                     /* 0x0048e960 */

void dMassSetCappedCylinder      ( dMass *m, dReal density, int direction,
                                   dReal radius, dReal length );     /* 0x0048e9e0 */
void dMassSetCappedCylinderTotal ( dMass *m, dReal total_mass, int direction,
                                   dReal radius, dReal length );     /* 0x0048eb30 */

void dMassSetCylinder      ( dMass *m, dReal density, int direction,
                             dReal radius, dReal length );           /* 0x0048eb70 */
void dMassSetCylinderTotal ( dMass *m, dReal total_mass, int direction,
                             dReal radius, dReal length );           /* 0x0048ebc0 */

void dMassSetBox      ( dMass *m, dReal density,
                        dReal lx, dReal ly, dReal lz );              /* 0x0048ec70 */
void dMassSetBoxTotal ( dMass *m, dReal total_mass,
                        dReal lx, dReal ly, dReal lz );              /* 0x0048ecb0 */

void dMassAdjust    ( dMass *m, dReal newmass );                     /* 0x0048ed60 */
void dMassTranslate ( dMass *m, dReal x, dReal y, dReal z );         /* 0x0048ee10 */
void dMassRotate    ( dMass *m, const dMatrix3 R );                  /* 0x0048f020 */
void dMassAdd       ( dMass *a, const dMass *b );                    /* 0x0048f0e0 */

/* Mass_ComputeProperties  0x0048f1c0 */
struct Entity_s;

typedef bool ( *MassInsideFunc_t )( const vec3_t point, const vec3_t halfCell,
                                    struct Entity_s *ent );

void Mass_ComputeProperties( const vec3_t mins, const vec3_t maxs,
                             MassInsideFunc_t inside, struct Entity_s *ent,
                             vec3_t centerOfMass, vec3_t momentOfInertia,
                             vec3_t productOfInertia );              /* 0x0048f1c0 */

#endif
