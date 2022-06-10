#pragma once

#include <cmath>
#include <iostream>
#include <cassert>

class Vec2 {
private:
    float mVals[2];
public:
    Vec2() 
    {
        mVals[0] = 0; mVals[1] = 0;
    }
    Vec2( float x , float y ) {
       mVals[0] = x; mVals[1] = y;
    }
    float & operator [] (unsigned int c) { return mVals[c]; }
    float operator [] (unsigned int c) const { return mVals[c]; }
    void operator = (Vec2 const & other) {
       mVals[0] = other[0] ; mVals[1] = other[1];
    }
    float squareLength() const {
       return mVals[0]*mVals[0] + mVals[1]*mVals[1];
    }
    float length() const { return sqrt( squareLength() ); }
    void normalize() { float L = length(); mVals[0] /= L; mVals[1] /= L; }
    static float dot( Vec2 const & a , Vec2 const & b ) {
       return a[0]*b[0] + a[1]*b[1];
    }
    void operator += (Vec2 const & other) {
        mVals[0] += other[0];
        mVals[1] += other[1];
    }
    void operator -= (Vec2 const & other) {
        mVals[0] -= other[0];
        mVals[1] -= other[1];
    }
    void operator *= (float s) {
        mVals[0] *= s;
        mVals[1] *= s;
    }
    void operator /= (float s) {
        mVals[0] /= s;
        mVals[1] /= s;
    }

    static Vec2 Rand(float magnitude = 1.f) {
        return Vec2( magnitude * (-1.f + 2.f * rand() / (float)(RAND_MAX)) , magnitude * (-1.f + 2.f * rand() / (float)(RAND_MAX)) );
    }
};

static inline Vec2 operator + (Vec2 const & a , Vec2 const & b) {
   return Vec2(a[0]+b[0] , a[1]+b[1]);
}
static inline Vec2 operator - (Vec2 const & a , Vec2 const & b) {
   return Vec2(a[0]-b[0] , a[1]-b[1]);
}
static inline Vec2 operator * (float a , Vec2 const & b) {
   return Vec2(a*b[0] , a*b[1]);
}
static inline Vec2 operator / (Vec2 const &  a , float b) {
   return Vec2(a[0]/b , a[1]/b);
}
static inline std::ostream & operator << (std::ostream & s , Vec2 const & p) {
    s << p[0] << " " << p[1];
    return s;
}
static inline std::istream & operator >> (std::istream & s , Vec2 & p) {
    s >> p[0] >> p[1];
    return s;
}

//-------------------------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------------------------------//

template< typename T >
class Mat2
{
public:
    typedef T   type_t;

    Mat2<T>()
    {
        vals[0] = 0;
        vals[1] = 0;
        vals[2] = 0;
        vals[3] = 0;
    }
    Mat2<T>( T v1 , T v2 , T v3 , T v4 )
    {
        vals[0] = v1;
        vals[1] = v2;
        vals[2] = v3;
        vals[3] = v4;
    }

    void set( const Mat2<T> & m )
    {
        vals[0] = m(0);
        vals[1] = m(1);
        vals[2] = m(2);
        vals[3] = m(3);
    }

    static
    Mat2<T> Id()
    {
        return Mat2<T>(1,0,0,1);
    }
    static
    Mat2<T> Zero()
    {
        return Mat2<T>(0,0,0,0);
    }
    void setZero()
    {
        vals[0] = 0;
        vals[1] = 0;
        vals[2] = 0;
        vals[3] = 0;
    }
    void setId()
    {
        vals[0] = 1;
        vals[1] = 0;
        vals[2] = 0;
        vals[3] = 1;
    }
    void setRotation(T angle)
    {
        T cangle = std::cos(angle);
        T sangle = std::sin(angle);
        vals[0] = cangle;
        vals[1] = -sangle;
        vals[2] = sangle;
        vals[3] = cangle;
    }

    Mat2<T> RPi_2()
    {
        return Mat2<T>(0,-1,1,0);
    }

    void transpose()
    {
        T xy = vals[1];
        vals[1] = vals[2];
        vals[2] = xy;
    }
    Mat2<T> getTranspose() const
    {
        return Mat2<T>(vals[0],vals[2],vals[1],vals[3]);
    }

    T operator () (unsigned int i , unsigned int j) const
    {
        return vals[2*i + j];
    }

    T operator () (unsigned int v) const
    {
        return vals[v];
    }
    T & operator () (unsigned int i , unsigned int j)
    {
        return vals[2*i + j];
    }

    T & operator () (unsigned int v)
    {
        return vals[v];
    }


    template< class point_t >
    point_t getRow( unsigned int i )
    {
        return point_t( vals[2*i] , vals[2*i+1] );
    }
    template< class point_t >
    void setRow( unsigned int i , const point_t & p)
    {
        vals[2*i]   = p[0];
        vals[2*i+1] = p[1];
    }

    template< class point_t >
    point_t getCol(unsigned int j)
    {
        return point_t( vals[j] , vals[2+j] );
    }
    template< class point_t >
    void setCol(unsigned int j , const point_t & p)
    {
        vals[j]   = p[0];
        vals[2+j] = p[1];
    }

    template< class point_t >
    inline static Mat2<T> getFromCols(const point_t & c1 , const point_t & c2 )
    {
        // 0 1
        // 2 3
        return Mat2<T>( c1[0] , c2[0] ,
                c1[1] , c2[1]  );
    }
    template< class point_t >
    inline static Mat2<T> getFromRows(const point_t & r1 , const point_t & r2 )
    {
        // 0 1
        // 2 3
        return Mat2<T>( r1[0] , r1[1] ,
                r2[0] , r2[1] );
    }

    inline
    T trace() const {
        return vals[0] + vals[3];
    }

    inline
    T determinant() const
    {
        return vals[0] * vals[3] - vals[1] * vals[2];
    }

    template< class point_t >
    void diagonalizeSymmetric( T & sx , T & sy , point_t & vx , point_t & vy )
    {
        T bb = vals[1]*vals[1] , aa = vals[0]*vals[0] , cc = vals[3]*vals[3];
        if( bb < 0.00001 * aa  &&  bb < 0.00001 * cc )
        {
            sx = vals[0];
            vx = point_t(1,0);
            sy = vals[3];
            vy = point_t(0,1);
            return;
        }
        T Trace = vals[0]+vals[3];
        T sqrtDelta = sqrt( ( vals[0]-vals[3] ) * ( vals[0]-vals[3] ) + 4.0 * vals[1]*vals[1] );
        sx = (Trace + sqrtDelta)/2.0;
        sy = (Trace - sqrtDelta)/2.0;
        T b = vals[1];
        {
            T aT = vals[0] - sx;
            T cT = vals[3] - sx;
            //T factor1 = -( aT*cT + b*b ) / (2.0 * b * cT);
            //T factor2 = -( aT*cT + b*b ) / (2.0 * b * aT);
            if( aT*aT  <  cT*cT )
            {
                // return estimate with x = 1 and y = -( aT*cT + b*b ) / (2.0 * b * cT) * x:
                vx[0] = 1.0;
                vx[1] = -( aT*cT + b*b ) / (2.0 * b * cT);
                vx.normalize();
            }
            else
            {
                // return estimate with y = 1 and x = -( aT*cT + b*b ) / (2.0 * b * aT) * y:
                vx[1] = 1.0;
                vx[0] = -( aT*cT + b*b ) / (2.0 * b * aT);
                vx.normalize();
            }
        }
        {
            T aT = vals[0] - sy;
            T cT = vals[3] - sy;
            //T factor1 = -( aT*cT + b*b ) / (2.0 * b * cT);
            //T factor2 = -( aT*cT + b*b ) / (2.0 * b * aT);
            if( aT*aT  <  cT*cT )
            {
                // return estimate with x = 1 and y = -( aT*cT + b*b ) / (2.0 * b * cT) * x:
                vy[0] = 1.0;
                vy[1] = -( aT*cT + b*b ) / (2.0 * b * cT);
                vy.normalize();
            }
            else
            {
                // return estimate with y = 1 and x = -( aT*cT + b*b ) / (2.0 * b * aT) * y:
                vy[1] = 1.0;
                vy[0] = -( aT*cT + b*b ) / (2.0 * b * aT);
                vy.normalize();
            }
        }
    }


    void diagonalizeSymmetric( T & sx , T & sy  )
    {
        Vec2  vx ,  vy;
        T bb = vals[1]*vals[1] , aa = vals[0]*vals[0] , cc = vals[3]*vals[3];
        if( bb < 0.00001 * aa  &&  bb < 0.00001 * cc )
        {
            sx = vals[0];
            vx = Vec2(1,0);
            sy = vals[3];
            vy = Vec2(0,1);
            return;
        }
        T Trace = vals[0]+vals[3];
        T sqrtDelta = sqrt( ( vals[0]-vals[3] ) * ( vals[0]-vals[3] ) + 4.0 * vals[1]*vals[1] );
        sx = (Trace + sqrtDelta)/2.0;
        sy = (Trace - sqrtDelta)/2.0;
        T b = vals[1];
        {
            T aT = vals[0] - sx;
            T cT = vals[3] - sx;
            //T factor1 = -( aT*cT + b*b ) / (2.0 * b * cT);
            //T factor2 = -( aT*cT + b*b ) / (2.0 * b * aT);
            if( aT*aT  <  cT*cT )
            {
                // return estimate with x = 1 and y = -( aT*cT + b*b ) / (2.0 * b * cT) * x:
                vx[0] = 1.0;
                vx[1] = -( aT*cT + b*b ) / (2.0 * b * cT);
                vx.normalize();
            }
            else
            {
                // return estimate with y = 1 and x = -( aT*cT + b*b ) / (2.0 * b * aT) * y:
                vx[1] = 1.0;
                vx[0] = -( aT*cT + b*b ) / (2.0 * b * aT);
                vx.normalize();
            }
        }
        {
            T aT = vals[0] - sy;
            T cT = vals[3] - sy;
            //T factor1 = -( aT*cT + b*b ) / (2.0 * b * cT);
            //T factor2 = -( aT*cT + b*b ) / (2.0 * b * aT);
            if( aT*aT  <  cT*cT )
            {
                // return estimate with x = 1 and y = -( aT*cT + b*b ) / (2.0 * b * cT) * x:
                vy[0] = 1.0;
                vy[1] = -( aT*cT + b*b ) / (2.0 * b * cT);
                vy.normalize();
            }
            else
            {
                // return estimate with y = 1 and x = -( aT*cT + b*b ) / (2.0 * b * aT) * y:
                vy[1] = 1.0;
                vy[0] = -( aT*cT + b*b ) / (2.0 * b * aT);
                vy.normalize();
            }
        }
    }

#ifdef __USE_GSL_FOR_Mat2
    void SVD( Mat2<T> & U , T & sx , T & sy , Mat2<T> & V )
    {
        gsl_matrix * u = gsl_matrix_alloc(2,2);
        for(unsigned int i = 0 ; i < 4; ++i)
            u->data[i] = vals[i];
        gsl_matrix * v = gsl_matrix_alloc(2,2);
        gsl_vector * s = gsl_vector_alloc(2);
        gsl_vector * work = gsl_vector_alloc(2);

        gsl_linalg_SV_decomp (u,
                              v,
                              s,
                              work);

        sx = s->data[0];
        sy = s->data[1];
        U(0) = u->data[0];
        U(1) = u->data[1];
        U(2) = u->data[2];
        U(3) = u->data[3];
        V(0) = v->data[0];
        V(1) = v->data[1];
        V(2) = v->data[2];
        V(3) = v->data[3];

        gsl_matrix_free(u);
        gsl_matrix_free(v);
        gsl_vector_free(s);
        gsl_vector_free(work);

        // a transformation T is given as R.B.S.Bt, R = rotation , B = local basis (rotation matrix), S = scales in the basis B
        // it can be obtained from the svd decomposition of T = U Sigma Vt :
        // B = V
        // S = Sigma
        // R = U.Vt
    }

    Mat2<T> getRotationalPart()
    {
        Mat2<T> U,V;
        T sx,sy;
        SVD(U,sx,sy,V);
        V.transpose();
        const Mat2<T> & res = U*V;
        if( res.determinant() < 0 )
        {
            U(0,0) = -U(0,0);
            U(0,1) = -U(0,1);
            return U*V;
        }
        // else
        return res;
    }

    void setRotation()
    {
        Mat2<T> U,V;
        T sx,sy;
        SVD(U,sx,sy,V);
        V.transpose();
        const Mat2<T> & res = U*V;
        if( res.determinant() < 0 )
        {
            U(0,0) = -U(0,0);
            U(0,1) = -U(0,1);
            set(U*V);
            return;
        }
        // else
        set(res);
    }

    void setSimilarity()
    {
        Mat2<T> U,V;
        T sx,sy;
        SVD(U,sx,sy,V);
        Mat2<T> S((sx+sy)/2,0,0,(sx+sy)/2);
        V.transpose();
        const Mat2<T> & res = U*S*V;
        if( res.determinant() < 0 )
        {
            U(0,0) = -U(0,0);
            U(0,1) = -U(0,1);
            set(U* S * V);
            return;
        }
        // else
        set(res);
    }

#endif

    void operator += (const Mat2<T> & m)
    {
        vals[0] += m(0);
        vals[1] += m(1);
        vals[2] += m(2);
        vals[3] += m(3);
    }
    void operator -= (const Mat2<T> & m)
    {
        vals[0] -= m(0);
        vals[1] -= m(1);
        vals[2] -= m(2);
        vals[3] -= m(3);
    }
    void operator /= (double t)
    {
        vals[0] /= t;
        vals[1] /= t;
        vals[2] /= t;
        vals[3] /= t;
    }

    T sqrnorm()
    {
        return vals[0]*vals[0] + vals[1]*vals[1] + vals[2]*vals[2] + vals[3]*vals[3];
    }

    template< class point_2D_t >
    point_2D_t inverse_vector( const point_2D_t & v ) const
    {
        T det = determinant();
        return point_2D_t( compute_determinant_with_c0_replaced(v[0],v[1])/det ,
                compute_determinant_with_c1_replaced(v[0],v[1])/det );
    }

    Mat2<T> getInverse() const {
        T det = determinant();
        return Mat2<T>( vals[3]/det , - vals[1]/det , - vals[2]/det , vals[0]/det );
    }



    template< class point_t >
    inline static
    Mat2<T> tensor( const point_t & p1 , const point_t & p2 )
    {
        return Mat2<T>(
                    p1[0]*p2[0] , p1[0]*p2[1] ,
                    p1[1]*p2[0] , p1[1]*p2[1] );
    }


    static
    Mat2<T> getRotationFromAngle( double u ) {
        T cosU = cos(u);
        T sinU = sin(u);
        return Mat2<T>( cosU , -sinU , sinU , cosU );
    }


private:
    T vals[4];
    // will be noted as :
    // 0 1
    // 2 3




    inline
    T compute_determinant_with_c0_replaced(T x , T y) const
    {
        return x * vals[3] - vals[1] * y;
    }

    inline
    T compute_determinant_with_c1_replaced(T x , T y) const
    {
        return vals[0] * y - x * vals[2];
    }




};

template< class T >
Mat2<T> operator + (const Mat2<T> & m1 , const Mat2<T> & m2)
{
    return Mat2<T>( m1(0)+m2(0) , m1(1)+m2(1) , m1(2)+m2(2) , m1(3)+m2(3) );
}
template< class T >
Mat2<T> operator - (const Mat2<T> & m1 , const Mat2<T> & m2)
{
    return Mat2<T>( m1(0)-m2(0) , m1(1)-m2(1) , m1(2)-m2(2) , m1(3)-m2(3) );
}

template< class T >
Mat2<T> operator * (int s , const Mat2<T> & m)
{
    return Mat2<T>( s*m(0) , s*m(1) , s*m(2) , s*m(3) );
}
template< class T >
Mat2<T> operator * (float s , const Mat2<T> & m)
{
    return Mat2<T>( s*m(0) , s*m(1) , s*m(2) , s*m(3) );
}
template< class T >
Mat2<T> operator * (double s , const Mat2<T> & m)
{
    return Mat2<T>( s*m(0) , s*m(1) , s*m(2) , s*m(3) );
}


template< class T >
Mat2<T> operator * (const Mat2<T> & m , int s)
{
    return Mat2<T>( s*m(0) , s*m(1) , s*m(2) , s*m(3) );
}
template< class T >
Mat2<T> operator * (const Mat2<T> & m , float s)
{
    return Mat2<T>( s*m(0) , s*m(1) , s*m(2) , s*m(3) );
}
template< class T >
Mat2<T> operator * (const Mat2<T> & m , double s)
{
    return Mat2<T>( s*m(0) , s*m(1) , s*m(2) , s*m(3) );
}


template< class T >
Mat2<T> operator / (const Mat2<T> & m , int s)
{
    return Mat2<T>( m(0)/s , m(1)/s , m(2)/s , m(3)/s );
}
template< class T >
Mat2<T> operator / (const Mat2<T> & m , float s)
{
    return Mat2<T>( m(0)/s , m(1)/s , m(2)/s , m(3)/s );
}
template< class T >
Mat2<T> operator / (const Mat2<T> & m , double s)
{
    return Mat2<T>( m(0)/s , m(1)/s , m(2)/s , m(3)/s );
}



template< typename T , class point_t >
point_t operator * (const Mat2<T> & m , const point_t & p) // computes m.p
{
    return point_t(
                m(0,0)*p[0] + m(0,1)*p[1] ,
                m(1,0)*p[0] + m(1,1)*p[1] );
}
template< typename T , class point_t >
point_t operator * (const point_t & p , const Mat2<T> & m) // computes p^t . m = (m^t . p)^t
{
    return point_t(
                m(0,0)*p[0] + m(1,0)*p[1] ,
                m(0,1)*p[0] + m(1,1)*p[1] );
}

template< typename T >
Mat2<T> operator * (const Mat2<T> & m1 , const Mat2<T> & m2)
{
    return Mat2<T>( m1(0,0)*m2(0,0) + m1(0,1)*m2(1,0) , m1(0,0)*m2(0,1) + m1(0,1)*m2(1,1),
                     m1(1,0)*m2(0,0) + m1(1,1)*m2(1,0) , m1(1,0)*m2(0,1) + m1(1,1)*m2(1,1));
}


typedef Mat2< float >  Mat2f;
typedef Mat2< double > Mat2d;
