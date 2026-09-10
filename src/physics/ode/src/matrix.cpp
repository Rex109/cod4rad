/* Original: ..\src\physics\ode\src\matrix.cpp */

#include "matrix.h"

/* dAllocaNotSupported  0x004906e0 */
void *dAllocaNotSupported( int size )
{
    AssertMsg( "don't use alloca" );
    return 0;
}

/* dSetZero  0x0048f8c0 */
void dSetZero (dReal *a, int n)
{
  dAASSERT (a && n >= 0);
  while (n > 0) {
    *(a++) = 0;
    n--;
  }
}

/* dSetValue  0x0048f910 */
void dSetValue (dReal *a, int n, dReal value)
{
  dAASSERT (a && n >= 0);
  while (n > 0) {
    *(a++) = value;
    n--;
  }
}

/* dMultiply0  0x0048f960 */
void dMultiply0 (dReal *A, const dReal *B, const dReal *C, int p, int q, int r)
{
  int i,j,k,qskip,rskip,rpad;
  dAASSERT (A && B && C && p>0 && q>0 && r>0);
  qskip = dPAD(q);
  rskip = dPAD(r);
  rpad = rskip - r;
  dReal sum;
  const dReal *b,*c,*bb;
  bb = B;
  for (i=p; i; i--) {
    for (j=0 ; j<r; j++) {
      c = C + j;
      b = bb;
      sum = 0;
      for (k=q; k; k--, c+=rskip) sum += (*(b++))*(*c);
      *(A++) = sum;
    }
    A += rpad;
    bb += qskip;
  }
}

/* dMultiply1  0x0048fac0 */
void dMultiply1 (dReal *A, const dReal *B, const dReal *C, int p, int q, int r)
{
  int i,j,k,pskip,rskip;
  dReal sum;
  dAASSERT (A && B && C && p>0 && q>0 && r>0);
  pskip = dPAD(p);
  rskip = dPAD(r);
  for (i=0; i<p; i++) {
    for (j=0; j<r; j++) {
      sum = 0;
      for (k=0; k<q; k++) sum += B[i+k*pskip] * C[j+k*rskip];
      A[i*rskip+j] = sum;
    }
  }
}

/* dMultiply2  0x0048fbf0 */
void dMultiply2 (dReal *A, const dReal *B, const dReal *C, int p, int q, int r)
{
  int i,j,k,z,rpad,qskip;
  dReal sum;
  const dReal *bb,*cc;
  dAASSERT (A && B && C && p>0 && q>0 && r>0);
  rpad = dPAD(r) - r;
  qskip = dPAD(q);
  bb = B;
  for (i=p; i; i--) {
    cc = C;
    for (j=r; j; j--) {
      z = 0;
      sum = 0;
      for (k=q; k; k--,z++) sum += bb[z] * cc[z];
      *(A++) = sum;
      cc += qskip;
    }
    A += rpad;
    bb += qskip;
  }
}

/* dDot  0x0048d6c0 */
dReal dDot (const dReal *a, const dReal *b, int n)
{
  dReal p0,q0,m0,p1,q1,m1,sum;
  sum = 0;
  n -= 2;
  while (n >= 0) {
    p0 = a[0]; q0 = b[0];
    m0 = p0 * q0;
    p1 = a[1]; q1 = b[1];
    m1 = p1 * q1;
    sum += m0;
    sum += m1;
    a += 2;
    b += 2;
    n -= 2;
  }
  n += 2;
  while (n > 0) {
    sum += (*a) * (*b);
    a++;
    b++;
    n--;
  }
  return sum;
}

/* dFactorCholesky3  0x0048fd50 */
int dFactorCholesky3 (dReal *A)
{
  const int n = 3;
  int i,j,k,nskip;
  dReal sum,*a,*b,*aa,*bb,*cc;
  dReal recip[3];
  dAASSERT (A);
  nskip = dPAD (n);
  aa = A;
  for (i=0; i<n; i++) {
    bb = A;
    cc = A + i*nskip;
    for (j=0; j<i; j++) {
      sum = *cc;
      a = aa;
      b = bb;
      for (k=j; k; k--) sum -= (*(a++))*(*(b++));
      *cc = sum * recip[j];
      bb += nskip;
      cc++;
    }
    sum = *cc;
    a = aa;
    for (k=i; k; k--, a++) sum -= (*a)*(*a);
    if (sum <= REAL(0.0)) return 0;
    *cc = dSqrt(sum);
    dIASSERT(*cc > REAL(0.0));
    recip[i] = dRecip (*cc);
    aa += nskip;
  }
  return 1;
}

/* dSolveCholesky3  0x0048ff20 */
void dSolveCholesky3 (const dReal *L, dReal *b)
{
  const int n = 3;
  int i,k,nskip;
  dReal sum,y[3];
  dAASSERT (L && b);
  nskip = dPAD (n);
  for (i=0; i<n; i++) {
    sum = 0;
    for (k=0; k < i; k++) sum += L[i*nskip+k]*y[k];
    dIASSERT(L[i * nskip + i ]);
    y[i] = (b[i]-sum)/L[i*nskip+i];
  }
  for (i=n-1; i >= 0; i--) {
    sum = 0;
    for (k=i+1; k < n; k++) sum += L[k*nskip+i]*b[k];
    dIASSERT(L[ i * nskip + i ]);
    b[i] = (y[i]-sum)/L[i*nskip+i];
  }
}

/* dInvertPDMatrix3  0x004900f0 */
int dInvertPDMatrix3 (const dReal *A, dReal *Ainv)
{
  const int n = 3;
  int i,j,nskip;
  dReal L[4*3];
  dReal x[3];
  dAASSERT (A && Ainv);
  nskip = dPAD (n);
  memcpy (L,A,nskip*n*sizeof(dReal));
  if (dFactorCholesky3 (L)==0) return 0;
  dSetZero (Ainv,n*nskip);
  for (i=0; i<n; i++) {
    for (j=0; j<n; j++) x[j] = 0;
    x[i] = 1;
    dSolveCholesky3 (L,x);
    for (j=0; j<n; j++) Ainv[j*nskip+i] = x[j];
  }
  return 1;
}

/* dIsPositiveDefinite3  0x00490210 */
int dIsPositiveDefinite3 (const dReal *A)
{
  const int n = 3;
  dReal Acopy[4*3];
  dAASSERT (A);
  memcpy (Acopy,A,sizeof(Acopy));
  return dFactorCholesky3 (Acopy);
}

/* dVectorScale  0x00490270 */
void dVectorScale (dReal *a, const dReal *d, int n)
{
  dAASSERT (a && d && n >= 0);
  for (int i=0; i<n; i++) a[i] *= d[i];
}

/* dSolveLDLT  0x004902f0 */
void dSolveLDLT (const dReal *L, const dReal *d, dReal *b, int n, int nskip)
{
  dAASSERT (L && d && b && n > 0 && nskip >= n);
  dSolveL1 (L,b,n,nskip);
  dVectorScale (b,d,n);
  dSolveL1T (L,b,n,nskip);
}

/* dSolveL1  0x0048d780 */
void dSolveL1 (const dReal *L, dReal *B, int n, int lskip1)
{
  dReal Z11,Z21,Z31,Z41,p1,q1,p2,p3,p4,*ex;
  const dReal *ell;
  int lskip2,lskip3,i,j;
  lskip2 = 2*lskip1;
  lskip3 = 3*lskip1;
  for (i=0; i <= n-4; i+=4) {
    Z11=0;
    Z21=0;
    Z31=0;
    Z41=0;
    ell = L + i*lskip1;
    ex = B;
    for (j=i-12; j >= 0; j -= 12) {
      p1=ell[0];
      q1=ex[0];
      p2=ell[lskip1];
      p3=ell[lskip2];
      p4=ell[lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[1];
      q1=ex[1];
      p2=ell[1+lskip1];
      p3=ell[1+lskip2];
      p4=ell[1+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[2];
      q1=ex[2];
      p2=ell[2+lskip1];
      p3=ell[2+lskip2];
      p4=ell[2+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[3];
      q1=ex[3];
      p2=ell[3+lskip1];
      p3=ell[3+lskip2];
      p4=ell[3+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[4];
      q1=ex[4];
      p2=ell[4+lskip1];
      p3=ell[4+lskip2];
      p4=ell[4+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[5];
      q1=ex[5];
      p2=ell[5+lskip1];
      p3=ell[5+lskip2];
      p4=ell[5+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[6];
      q1=ex[6];
      p2=ell[6+lskip1];
      p3=ell[6+lskip2];
      p4=ell[6+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[7];
      q1=ex[7];
      p2=ell[7+lskip1];
      p3=ell[7+lskip2];
      p4=ell[7+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[8];
      q1=ex[8];
      p2=ell[8+lskip1];
      p3=ell[8+lskip2];
      p4=ell[8+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[9];
      q1=ex[9];
      p2=ell[9+lskip1];
      p3=ell[9+lskip2];
      p4=ell[9+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[10];
      q1=ex[10];
      p2=ell[10+lskip1];
      p3=ell[10+lskip2];
      p4=ell[10+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      p1=ell[11];
      q1=ex[11];
      p2=ell[11+lskip1];
      p3=ell[11+lskip2];
      p4=ell[11+lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      ell += 12;
      ex += 12;
    }
    j += 12;
    for (; j > 0; j--) {
      p1=ell[0];
      q1=ex[0];
      p2=ell[lskip1];
      p3=ell[lskip2];
      p4=ell[lskip3];
      Z11 += p1 * q1;
      Z21 += p2 * q1;
      Z31 += p3 * q1;
      Z41 += p4 * q1;
      ell += 1;
      ex += 1;
    }
    Z11 = ex[0] - Z11;
    ex[0] = Z11;
    p1 = ell[lskip1];
    Z21 = ex[1] - Z21 - p1*Z11;
    ex[1] = Z21;
    p1 = ell[lskip2];
    p2 = ell[1+lskip2];
    Z31 = ex[2] - Z31 - p1*Z11 - p2*Z21;
    ex[2] = Z31;
    p1 = ell[lskip3];
    p2 = ell[1+lskip3];
    p3 = ell[2+lskip3];
    Z41 = ex[3] - Z41 - p1*Z11 - p2*Z21 - p3*Z31;
    ex[3] = Z41;
  }
  for (; i < n; i++) {
    Z11=0;
    ell = L + i*lskip1;
    ex = B;
    for (j=i-12; j >= 0; j -= 12) {
      p1=ell[0];
      q1=ex[0];
      Z11 += p1 * q1;
      p1=ell[1];
      q1=ex[1];
      Z11 += p1 * q1;
      p1=ell[2];
      q1=ex[2];
      Z11 += p1 * q1;
      p1=ell[3];
      q1=ex[3];
      Z11 += p1 * q1;
      p1=ell[4];
      q1=ex[4];
      Z11 += p1 * q1;
      p1=ell[5];
      q1=ex[5];
      Z11 += p1 * q1;
      p1=ell[6];
      q1=ex[6];
      Z11 += p1 * q1;
      p1=ell[7];
      q1=ex[7];
      Z11 += p1 * q1;
      p1=ell[8];
      q1=ex[8];
      Z11 += p1 * q1;
      p1=ell[9];
      q1=ex[9];
      Z11 += p1 * q1;
      p1=ell[10];
      q1=ex[10];
      Z11 += p1 * q1;
      p1=ell[11];
      q1=ex[11];
      Z11 += p1 * q1;
      ell += 12;
      ex += 12;
    }
    j += 12;
    for (; j > 0; j--) {
      p1=ell[0];
      q1=ex[0];
      Z11 += p1 * q1;
      ell += 1;
      ex += 1;
    }
    Z11 = ex[0] - Z11;
    ex[0] = Z11;
  }
}

/* dSolveL1T  0x0048e0a0 */
void dSolveL1T (const dReal *L, dReal *B, int n, int lskip1)
{
  dReal Z11,m11,Z21,m21,Z31,m31,Z41,m41,p1,q1,p2,p3,p4,*ex;
  const dReal *ell;
  int lskip2,lskip3,i,j;
  L = L + (n-1)*(lskip1+1);
  B = B + n-1;
  lskip1 = -lskip1;
  lskip2 = 2*lskip1;
  lskip3 = 3*lskip1;
  for (i=0; i <= n-4; i+=4) {
    Z11=0;
    Z21=0;
    Z31=0;
    Z41=0;
    ell = L - i;
    ex = B;
    for (j=i-4; j >= 0; j -= 4) {
      p1=ell[0];
      q1=ex[0];
      p2=ell[-1];
      p3=ell[-2];
      p4=ell[-3];
      m11 = p1 * q1;
      m21 = p2 * q1;
      m31 = p3 * q1;
      m41 = p4 * q1;
      ell += lskip1;
      Z11 += m11;
      Z21 += m21;
      Z31 += m31;
      Z41 += m41;
      p1=ell[0];
      q1=ex[-1];
      p2=ell[-1];
      p3=ell[-2];
      p4=ell[-3];
      m11 = p1 * q1;
      m21 = p2 * q1;
      m31 = p3 * q1;
      m41 = p4 * q1;
      ell += lskip1;
      Z11 += m11;
      Z21 += m21;
      Z31 += m31;
      Z41 += m41;
      p1=ell[0];
      q1=ex[-2];
      p2=ell[-1];
      p3=ell[-2];
      p4=ell[-3];
      m11 = p1 * q1;
      m21 = p2 * q1;
      m31 = p3 * q1;
      m41 = p4 * q1;
      ell += lskip1;
      Z11 += m11;
      Z21 += m21;
      Z31 += m31;
      Z41 += m41;
      p1=ell[0];
      q1=ex[-3];
      p2=ell[-1];
      p3=ell[-2];
      p4=ell[-3];
      m11 = p1 * q1;
      m21 = p2 * q1;
      m31 = p3 * q1;
      m41 = p4 * q1;
      ell += lskip1;
      Z11 += m11;
      Z21 += m21;
      Z31 += m31;
      Z41 += m41;
      ex -= 4;
    }
    j += 4;
    for (; j > 0; j--) {
      p1=ell[0];
      q1=ex[0];
      p2=ell[-1];
      p3=ell[-2];
      p4=ell[-3];
      m11 = p1 * q1;
      m21 = p2 * q1;
      m31 = p3 * q1;
      m41 = p4 * q1;
      ell += lskip1;
      ex -= 1;
      Z11 += m11;
      Z21 += m21;
      Z31 += m31;
      Z41 += m41;
    }
    Z11 = ex[0] - Z11;
    ex[0] = Z11;
    p1 = ell[-1];
    Z21 = ex[-1] - Z21 - p1*Z11;
    ex[-1] = Z21;
    p1 = ell[-2];
    p2 = ell[-2+lskip1];
    Z31 = ex[-2] - Z31 - p1*Z11 - p2*Z21;
    ex[-2] = Z31;
    p1 = ell[-3];
    p2 = ell[-3+lskip1];
    p3 = ell[-3+lskip2];
    Z41 = ex[-3] - Z41 - p1*Z11 - p2*Z21 - p3*Z31;
    ex[-3] = Z41;
  }
  for (; i < n; i++) {
    Z11=0;
    ell = L - i;
    ex = B;
    for (j=i-4; j >= 0; j -= 4) {
      p1=ell[0];
      q1=ex[0];
      m11 = p1 * q1;
      ell += lskip1;
      Z11 += m11;
      p1=ell[0];
      q1=ex[-1];
      m11 = p1 * q1;
      ell += lskip1;
      Z11 += m11;
      p1=ell[0];
      q1=ex[-2];
      m11 = p1 * q1;
      ell += lskip1;
      Z11 += m11;
      p1=ell[0];
      q1=ex[-3];
      m11 = p1 * q1;
      ell += lskip1;
      Z11 += m11;
      ex -= 4;
    }
    j += 4;
    for (; j > 0; j--) {
      p1=ell[0];
      q1=ex[0];
      m11 = p1 * q1;
      ell += lskip1;
      ex -= 1;
      Z11 += m11;
    }
    Z11 = ex[0] - Z11;
    ex[0] = Z11;
  }
}

/* dLDLTAddTL  0x00490380 */
void dLDLTAddTL (dReal *L, dReal *d, const dReal *a, int n, int nskip)
{
  int j,p;
  dReal *W1,*W2,W11,W21,alpha1,alpha2,alphanew,gamma1,gamma2,k1,k2,Wp,ell,dee;

  dAASSERT (L && d && a && n > 0 && nskip >= n);
  if (n < 2) return;
  W1 = (dReal*) ALLOCA (n*sizeof(dReal));
  W2 = (dReal*) ALLOCA (n*sizeof(dReal));

  W1[0] = 0;
  W2[0] = 0;
  for (j=1; j<n; j++) W1[j] = W2[j] = (dReal) (a[j] * M_SQRT1_2);
  W11 = (dReal) ((REAL(0.5)*a[0]+1)*M_SQRT1_2);
  W21 = (dReal) ((REAL(0.5)*a[0]-1)*M_SQRT1_2);

  alpha1 = 1;
  alpha2 = 1;

  dee = d[0];
  alphanew = alpha1 + (W11*W11)*dee;
  dee /= alphanew;
  gamma1 = W11 * dee;
  dee *= alpha1;
  alpha1 = alphanew;
  alphanew = alpha2 - (W21*W21)*dee;
  dee /= alphanew;
  gamma2 = W21 * dee;
  alpha2 = alphanew;
  k1 = REAL(1.0) - W21*gamma1;
  k2 = W21*gamma1*W11 - W21;
  for (p=1; p<n; p++) {
    Wp = W1[p];
    ell = L[p*nskip];
    W1[p] =    Wp - W11*ell;
    W2[p] = k1*Wp +  k2*ell;
  }

  for (j=1; j<n; j++) {
    dReal *ll = L + (j*nskip) + j;
    W11 = W1[j];
    W21 = W2[j];
    dee = d[j];
    alphanew = alpha1 + (W11*W11)*dee;
    dee /= alphanew;
    gamma1 = W11 * dee;
    dee *= alpha1;
    alpha1 = alphanew;
    alphanew = alpha2 - (W21*W21)*dee;
    dee /= alphanew;
    gamma2 = W21 * dee;
    dee *= alpha2;
    d[j] = dee;
    alpha2 = alphanew;
    k1 = W11;
    k2 = W21;
    for (p=j+1; p<n; p++) {
      ell = ll[(p-j)*nskip];
      Wp = W1[p] - k1 * ell;
      ell += gamma1 * Wp;
      W1[p] = Wp;
      Wp = W2[p] - k2 * ell;
      ell -= gamma2 * Wp;
      W2[p] = Wp;
      ll[(p-j)*nskip] = ell;
    }
  }
}

#define _GETA(i,j) (A[i][j])
#define GETA(i,j) ((i > j) ? _GETA(i,j) : _GETA(j,i))

/* dLDLTRemove  0x00490710 */
void dLDLTRemove (dReal **A, const int *p, dReal *L, dReal *d,
		  int n1, int n2, int r, int nskip)
{
  int i;
  dAASSERT(A && p && L && d && n1 > 0 && n2 > 0 && r >= 0 && r < n2 &&
	   n1 >= n2 && nskip >= n1);
# ifndef dNODEBUG
  for (i=0; i<n2; i++) dIASSERT(p[i] >= 0 && p[i] < n1);
# endif

  if (r==n2-1) {
    return;
  }
  else if (r==0) {
    dReal *a = (dReal*) ALLOCA (n2*sizeof(dReal));
    for (i=0; i<n2; i++) a[i] = -GETA(p[i],p[0]);
    a[0] += REAL(1.0);
    dLDLTAddTL (L,d,a,n2,nskip);
  }
  else {
    dReal *t = (dReal*) ALLOCA (r*sizeof(dReal));
    dReal *a = (dReal*) ALLOCA ((n2-r)*sizeof(dReal));
    for (i=0; i<r; i++) t[i] = L[r*nskip+i] / d[i];
    for (i=0; i<(n2-r); i++)
      a[i] = dDot(L+(r+i)*nskip,t,r) - GETA(p[r+i],p[r]);
    a[0] += REAL(1.0);
    dLDLTAddTL (L + r*nskip+r, d+r, a, n2-r, nskip);
  }

  dRemoveRowCol (L,n2,nskip,r);
  if (r < (n2-1)) memmove (d+r,d+r+1,(n2-r-1)*sizeof(dReal));
}

/* dRemoveRowCol  0x00490a50 */
void dRemoveRowCol (dReal *A, int n, int nskip, int r)
{
  int i;
  dAASSERT(A && n > 0 && nskip >= n && r >= 0 && r < n);
  if (r >= n-1) return;
  if (r > 0) {
    for (i=0; i<r; i++)
      memmove (A+i*nskip+r,A+i*nskip+r+1,(n-r-1)*sizeof(dReal));
    for (i=r; i<(n-1); i++)
      memcpy (A+i*nskip,A+i*nskip+nskip,r*sizeof(dReal));
  }
  for (i=r; i<(n-1); i++)
    memcpy (A+i*nskip+r,A+i*nskip+nskip+r+1,(n-r-1)*sizeof(dReal));
}


/* dDOT  0x0048f830 */
dReal dDOT (const dReal *a, const dReal *b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

/* dDOT14  0x0048f700 */
dReal dDOT14 (const dReal *a, const dReal *b)
{
  return a[0]*b[0] + a[1]*b[4] + a[2]*b[8];
}

/* dMultiply0_331  0x0048f870 */
void dMultiply0_331 (dReal *A, const dReal *B, const dReal *C)
{
  A[0] = dDOT(B,C);
  A[1] = dDOT(B+4,C);
  A[2] = dDOT(B+8,C);
}

/* dMultiply0_333  0x0048f610 */
void dMultiply0_333 (dReal *A, const dReal *B, const dReal *C)
{
  A[0]  = dDOT14(B,C);
  A[1]  = dDOT14(B,C+1);
  A[2]  = dDOT14(B,C+2);
  A[4]  = dDOT14(B+4,C);
  A[5]  = dDOT14(B+4,C+1);
  A[6]  = dDOT14(B+4,C+2);
  A[8]  = dDOT14(B+8,C);
  A[9]  = dDOT14(B+8,C+1);
  A[10] = dDOT14(B+8,C+2);
}

/* dMultiply2_333  0x0048f740 */
void dMultiply2_333 (dReal *A, const dReal *B, const dReal *C)
{
  A[0]  = dDOT(B,C);
  A[1]  = dDOT(B,C+4);
  A[2]  = dDOT(B,C+8);
  A[4]  = dDOT(B+4,C);
  A[5]  = dDOT(B+4,C+4);
  A[6]  = dDOT(B+4,C+8);
  A[8]  = dDOT(B+8,C);
  A[9]  = dDOT(B+8,C+4);
  A[10] = dDOT(B+8,C+8);
}
