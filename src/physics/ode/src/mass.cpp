/* Original: ..\src\physics\ode\src\mass.cpp */

#include "mass.h"

/* checkMass  0x0048e780 */
static void checkMass (dMass *m)
{
  int i;
  dAASSERT (m->mass > 0);
  dAASSERT (dIsPositiveDefinite3( m->I ));
  dMatrix3 I2,chat;
  dSetZero (chat,12);
  dCROSSMAT (chat,m->c,4,+,-);
  dMultiply0_333 (I2,chat,chat);
  for (i=0; i<3; i++) I2[i] = m->I[i] + m->mass*I2[i];
  for (i=4; i<7; i++) I2[i] = m->I[i] + m->mass*I2[i];
  for (i=8; i<11; i++) I2[i] = m->I[i] + m->mass*I2[i];
  dUASSERT (dIsPositiveDefinite3( I2 ),
	    "center of mass inconsistent with mass parameters");
}

/* dMassSetZero  0x0048e660 */
void dMassSetZero (dMass *m)
{
  dAASSERT (m);
  m->mass = REAL(0.0);
  dSetZero (m->c,sizeof(m->c) / sizeof(dReal));
  dSetZero (m->I,sizeof(m->I) / sizeof(dReal));
}

/* dMassSetParameters  0x0048e6c0 */
void dMassSetParameters (dMass *m, dReal themass,
			 dReal cgx, dReal cgy, dReal cgz,
			 dReal I11, dReal I22, dReal I33,
			 dReal I12, dReal I13, dReal I23)
{
  dAASSERT (m);
  dMassSetZero (m);
  m->mass = themass;
  m->c[0] = cgx;
  m->c[1] = cgy;
  m->c[2] = cgz;
  m->_I(0,0) = I11;
  m->_I(1,1) = I22;
  m->_I(2,2) = I33;
  m->_I(0,1) = I12;
  m->_I(0,2) = I13;
  m->_I(1,2) = I23;
  m->_I(1,0) = I12;
  m->_I(2,0) = I13;
  m->_I(2,1) = I23;
  checkMass (m);
}

/* dMassSetSphere  0x0048e920 */
void dMassSetSphere (dMass *m, dReal density, dReal radius)
{
  dMassSetSphereTotal (m, (dReal) ((4.0/3.0) * M_PI * radius*radius*radius * density),
		       radius);
}

/* dMassSetSphereTotal  0x0048e960 */
void dMassSetSphereTotal (dMass *m, dReal total_mass, dReal radius)
{
  dAASSERT (m);
  dMassSetZero (m);
  m->mass = total_mass;
  m->_I(0,0) = m->_I(1,1) = m->_I(2,2) = REAL(0.4) * total_mass * radius*radius;
  checkMass (m);
}

/* dMassSetCappedCylinder  0x0048e9e0 */
void dMassSetCappedCylinder (dMass *m, dReal density, int direction,
			     dReal radius, dReal length)
{
  dReal M1,M2,Ia,Ib;
  dAASSERT (m);
  dUASSERT (direction >= 1 && direction <= 3,"bad direction number");
  dMassSetZero (m);
  M1 = M_PI*radius*radius*length*density;
  M2 = (dReal) ((4.0/3.0)*M_PI*radius*radius*radius*density);
  m->mass = M1+M2;
  Ia = M1*(REAL(0.25)*radius*radius + (1.0/12.0)*length*length) +
    M2*(REAL(0.4)*radius*radius + REAL(0.375)*radius*length + REAL(0.25)*length*length);
  Ib = (M1*REAL(0.5) + M2*REAL(0.4))*radius*radius;
  m->_I(0,0) = Ia;
  m->_I(1,1) = Ia;
  m->_I(2,2) = Ia;
  m->_I(direction-1,direction-1) = Ib;
  checkMass (m);
}

/* dMassSetCappedCylinderTotal  0x0048eb30 */
void dMassSetCappedCylinderTotal (dMass *m, dReal total_mass, int direction,
				  dReal radius, dReal length)
{
  dMassSetCappedCylinder (m, REAL(1.0), direction, radius, length);
  dMassAdjust (m, total_mass);
}

/* dMassSetCylinder  0x0048eb70 */
void dMassSetCylinder (dMass *m, dReal density, int direction,
		       dReal radius, dReal length)
{
  dMassSetCylinderTotal (m, M_PI*radius*radius*length*density,
			 direction, radius, length);
}

/* dMassSetCylinderTotal  0x0048ebc0 */
void dMassSetCylinderTotal (dMass *m, dReal total_mass, int direction,
			    dReal radius, dReal length)
{
  dReal r2,I;
  dAASSERT (m);
  dMassSetZero (m);
  r2 = radius*radius;
  m->mass = total_mass;
  I = total_mass*(REAL(0.25)*r2 + (1.0/12.0)*length*length);
  m->_I(0,0) = I;
  m->_I(1,1) = I;
  m->_I(2,2) = I;
  m->_I(direction-1,direction-1) = total_mass*REAL(0.5)*r2;
  checkMass (m);
}

/* dMassSetBox  0x0048ec70 */
void dMassSetBox (dMass *m, dReal density, dReal lx, dReal ly, dReal lz)
{
  dMassSetBoxTotal (m, lx*ly*lz*density, lx, ly, lz);
}

/* dMassSetBoxTotal  0x0048ecb0 */
void dMassSetBoxTotal (dMass *m, dReal total_mass, dReal lx, dReal ly, dReal lz)
{
  dAASSERT (m);
  dMassSetZero (m);
  m->mass = total_mass;
  m->_I(0,0) = total_mass/REAL(12.0) * (ly*ly + lz*lz);
  m->_I(1,1) = total_mass/REAL(12.0) * (lx*lx + lz*lz);
  m->_I(2,2) = total_mass/REAL(12.0) * (lx*lx + ly*ly);
  checkMass (m);
}

/* dMassAdjust  0x0048ed60 */
void dMassAdjust (dMass *m, dReal newmass)
{
  dAASSERT (m);
  dReal scale = newmass / m->mass;
  m->mass = newmass;
  for (int i=0; i<3; i++) for (int j=0; j<3; j++) m->_I(i,j) *= scale;
  checkMass (m);
}

/* dMassTranslate  0x0048ee10 */
void dMassTranslate (dMass *m, dReal x, dReal y, dReal z)
{
  int i,j;
  dMatrix3 ahat,chat,t1,t2;
  dReal a[3];
  dAASSERT (m);

  dSetZero (chat,12);
  dCROSSMAT (chat,m->c,4,+,-);
  a[0] = x + m->c[0];
  a[1] = y + m->c[1];
  a[2] = z + m->c[2];
  dSetZero (ahat,12);
  dCROSSMAT (ahat,a,4,+,-);
  dMultiply0_333 (t1,ahat,ahat);
  dMultiply0_333 (t2,chat,chat);
  for (i=0; i<3; i++) for (j=0; j<3; j++)
    m->_I(i,j) += m->mass * (t2[i*4+j]-t1[i*4+j]);
  m->_I(1,0) = m->_I(0,1);
  m->_I(2,0) = m->_I(0,2);
  m->_I(2,1) = m->_I(1,2);

  m->c[0] += x;
  m->c[1] += y;
  m->c[2] += z;

  checkMass (m);
}

/* dMassRotate  0x0048f020 */
void dMassRotate (dMass *m, const dMatrix3 R)
{
  dMatrix3 t1;
  dReal t2[3];
  dAASSERT (m);

  dMultiply2_333 (t1,m->I,R);
  dMultiply0_333 (m->I,R,t1);
  m->_I(1,0) = m->_I(0,1);
  m->_I(2,0) = m->_I(0,2);
  m->_I(2,1) = m->_I(1,2);

  dMultiply0_331 (t2,R,m->c);
  m->c[0] = t2[0];
  m->c[1] = t2[1];
  m->c[2] = t2[2];

  checkMass (m);
}

/* dMassAdd  0x0048f0e0 */
void dMassAdd (dMass *a, const dMass *b)
{
  int i;
  dAASSERT (a && b);
  dReal denom = dRecip (a->mass + b->mass);
  for (i=0; i<3; i++) a->c[i] = (a->c[i]*a->mass + b->c[i]*b->mass)*denom;
  a->mass += b->mass;
  for (i=0; i<12; i++) a->I[i] += b->I[i];
}


#include "../../../universal/com_vector.h"

/* Sqr  0x0048f5d0 */
static float Sqr( float x )
{
    return x * x;
}

/* Mass_ComputeProperties  0x0048f1c0 */
void Mass_ComputeProperties( const vec3_t mins, const vec3_t maxs,
                             MassInsideFunc_t inside, struct Entity_s *ent,
                             vec3_t centerOfMass, vec3_t momentOfInertia,
                             vec3_t productOfInertia )
{
    const int    steps = 32;
    dMass        mass;
    float        stepX, stepY, stepZ;
    vec3_t       halfCell;
    vec3_t       center;
    vec3_t       point;
    unsigned int count;
    unsigned int i, j, k;
    float        Ixx, Iyy, Izz;
    float        Ixy, Ixz, Iyz;
    float        invCount;

    stepX = ( maxs[0] - mins[0] ) / steps;
    stepY = ( maxs[1] - mins[1] ) / steps;
    stepZ = ( maxs[2] - mins[2] ) / steps;

    Vec3Set( halfCell, stepX * 0.5f, stepY * 0.5f, stepZ * 0.5f );

    count = 0;
    Vec3Clear( center );

    Izz = 0.0f;
    Iyy = 0.0f;
    Ixx = 0.0f;
    Ixy = 0.0f;
    Ixz = 0.0f;
    Iyz = 0.0f;

    point[0] = mins[0] + halfCell[0];
    for ( i = 0; i < steps; i++ )
    {
        point[1] = mins[1] + halfCell[1];
        for ( j = 0; j < steps; j++ )
        {
            point[2] = mins[2] + halfCell[2];
            for ( k = 0; k < steps; k++ )
            {
                if ( inside( point, halfCell, ent ) )
                {
                    count++;
                    Vec3Add( center, point, center );

                    Izz = Sqr( point[0] ) + Sqr( point[1] ) + Izz;
                    Iyy = Sqr( point[0] ) + Sqr( point[2] ) + Iyy;
                    Ixx = Sqr( point[1] ) + Sqr( point[2] ) + Ixx;

                    Ixy = point[0] * point[1] + Ixy;
                    Ixz = point[0] * point[2] + Ixz;
                    Iyz = point[1] * point[2] + Iyz;
                }
                point[2] = point[2] + stepZ;
            }
            point[1] = point[1] + stepY;
        }
        point[0] = point[0] + stepX;
    }

    if ( count != 0 )
    {
        invCount = 1.0f / count;
        Vec3Scale( center, invCount, centerOfMass );

        dMassSetParameters( &mass, 1.0f,
                            centerOfMass[0], centerOfMass[1], centerOfMass[2],
                            Ixx * invCount, Iyy * invCount, Izz * invCount,
                            -Ixy * invCount, -Ixz * invCount, -Iyz * invCount );

        dMassTranslate( &mass, -centerOfMass[0], -centerOfMass[1], -centerOfMass[2] );

        Vec3Set( momentOfInertia,  mass._I(0,0), mass._I(1,1), mass._I(2,2) );
        Vec3Set( productOfInertia, mass._I(0,1), mass._I(0,2), mass._I(1,2) );
    }
}
