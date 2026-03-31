#pragma once
//==============================================================================================
//  point.h                     independent point [***..]
//  CRUXELL.RND.software		jin lee
//  2018.02.28					2018.03.04
//------------------------------+---------------------------------------------------------------
//  indefines

namespace                       crux
{
//#define ttt                     template< typename T >
#define tt                      template
#define co                      const
#define cex                     constexpr
#define tn                      typename
#define CRE

using   f32                     = float;
using   f64                     = double;
using   STR                     = std::string;
using   WSTR					= std::wstring;

//------------------------------+---------------------------------------------------------------

cex f64 pi                      = 3.141592653589793238462643383279502884197169;
cex f64 r2d                     = 57.29577951308232087679815481410517033240547;
cex f64 d2r                     = 0.017453292519943295769236907684886127134428;
cex f64 exp1                    = 2.718281828459045235360287471352662497757247;
cex f64 sqrt2                   = 1.414213562373095048801688724209698078569671;
cex f64 sqrt3                   = 1.732050807568877293527446341505872366942805;
cex f64 sqrt5                   = 2.236067977499789696409173668731276235440618;
cex f64 sqrt7                   = 2.645751311064590590501615753639260425710259;
cex f64 sqrt11                  = 3.316624790355399849114932736670686683927088;

//------------------------------+---------------------------------------------------------------
//  angle types

enum    angle_type              { _radian, _degree };

tt< angle_type A >  struct      angle_
{
    f64                         angle;
    ttt CRE cex                 angle_( T&& r )     : angle( r ) {}
    ttt CRE cex                 angle_( co T& r )   : angle( r ) {}

    tt< angle_type B >  CRE cex angle_( angle_<B>&& r )
    {
        if( A != B )    angle = r.angle * ( A == _radian ? d2r : r2d );
        else    angle = r.angle;
    }
    tt< angle_type B >  CRE cex angle_( co angle_<B>& r )
    {
        if( A != B )    angle = r.angle * ( A == _radian ? d2r : r2d );
        else    angle = r.angle;
    }
    auto                        str() co
    {
        return  STRING( angle ) + ( A ? " deg" : "rad" );
    }
};

using                           rad = angle_< _radian >;
using                           deg = angle_< _degree >;

//------------------------------+---------------------------------------------------------------
//  abstraction of the point

ttt class AFX_EXT_CLASS         AB
{
    using                       C = AB<T>;

protected:
    std::array<T,2>             v;

public:
    ///-------------------------+---------------------------------------------------------------

    auto&                       a() { return  v[ 0 ]; }
    auto&                       b() { return  v[ 1 ]; }
    auto                        a() co { return  v[ 0 ]; }
    auto                        b() co { return  v[ 1 ]; }

    CRE                     cex AB() {}
    CRE                     cex AB( C&& r )             : v{ r.v } {}
    CRE                     cex AB( co C& r )           : v{ r.v } {}
    CRE tt< tn U >          cex AB( AB<U>&& r )         : v{ (T)r.a(), (T)r.b() } {}
    CRE tt< tn U >          cex AB( co AB<U>& r )       : v{ (T)r.a(), (T)r.b() } {}
    CRE tt< tn U, tn V >    cex AB( U&& a, V&& b )      : v{ (T)a, (T)b } {}
    CRE tt< tn U, tn V >    cex AB( co U& a, co V& b )  : v{ (T)a, (T)b } {}

    ///-------------------------+---------------------------------------------------------------

    auto&                       operator=( co C& r )
    {
        return  ( this->v = r.v ), *this;
    }

    auto&                       operator=( C&& r )
    {
        return  ( this->v = r.v ), *this;
    }

    auto                        str() co
    {
        return  STR( "[ " ) + std::to_string( a() ) + ", " + std::to_string( b() ) + " ]";
    }
};
//------------------------------+---------------------------------------------------------------
//  reusable roller for vector2d (XY)

ttt using                       roller  = AB<T>;

tt< tn T = f64 >    co          auto    make_roller( rad&& r )
{
    return  roller<T>{ std::sin( r.angle ), std::cos( r.angle ) };
}

//------------------------------+---------------------------------------------------------------
//  begin to end

ttt                     class   RANGE : public AB<T>
{
    using                       A = AB<T>;

public:
    auto&                       s() { return  A::a(); }
    auto&                       e() { return  A::b(); }
    auto                        s() co { return  A::a(); }
    auto                        e() co { return  A::b(); }

    CRE                     cex RANGE() {};
    CRE tt< tn U >          cex RANGE( AB<U>&& r )        : A{ r }      {}
    CRE tt< tn U >          cex RANGE( co AB<U>& r )      : A{ r }      {}
    CRE tt< tn U, tn V >    cex RANGE( U&& s, V&& e )     : A{ s, e }   {}
    CRE tt< tn U, tn V >    cex RANGE( co U& s, co V& e ) : A{ s, e }   {}

    auto                        size() co { return  e() - s(); }
};

//------------------------------+---------------------------------------------------------------
//  point / vector2d

ttt class                       XY : public AB< T >
{
    using                       A = AB<T>;
    using                       C = XY<T>;

public:
    ///-------------------------+---------------------------------------------------------------

    auto&                       x() { return  A::a(); }
    auto&                       y() { return  A::b(); }
    auto                        x() co { return  A::a(); }
    auto                        y() co { return  A::b(); }

    CRE                     cex XY() {}
    CRE                     cex XY( C&& r )             : A{ r }    {}
    CRE                     cex XY( co C& r )           : A{ r }    {}
    CRE tt< tn U >          cex XY( XY<U>&& r )         : A{ r.x(), r.y() } {}
    CRE tt< tn U >          cex XY( co XY<U>& r )       : A{ r.x(), r.y() } {}
    CRE tt< tn U >          cex XY( AB<U>&& r )         : A{ r.a(), r.b() } {}
    CRE tt< tn U >          cex XY( co AB<U>& r )       : A{ r.a(), r.b() } {}
    CRE tt< tn U, tn V >    cex XY( U&& a, V&& b )      : A{ a, b } {}
    CRE tt< tn U, tn V >    cex XY( co U& a, co V& b )  : A{ a, b } {}

    ///-------------------------+---------------------------------------------------------------

    auto&                       operator=( co C& r )
    {
        return  ( this->v = r.v ), *this;
    }

    auto&                       operator=( C&& r )
    {
        return  ( this->v = r.v ), *this;
    }
    ///-------------------------+---------------------------------------------------------------

    auto&                       operator+=( co C& r )
    {
        return  ( this->x() += r.x(), this->y() += r.y() ), *this;
    }

    auto&                       operator-=( co C& r )
    {
        return  ( this->x() -= r.x(), this->y() -= r.y() ), *this;
    }

    auto&                       operator*=( co C& r )
    {
        return  ( this->x() *= r.x(), this->y() *= r.y() ), *this;
    }

    auto&                       operator/=( co C& r )
    {
        return  ( this->x() /= r.x(), this->y() /= r.y() ), *this;
    }
    ///-------------------------+---------------------------------------------------------------

    auto                        operator+( co C& r ) co
    {
        return  C( *this ) += r;
    }

    auto                        operator-( co C& r ) co
    {
        return  C( *this ) -= r;
    }

    auto                        operator*( co C& r ) co
    {
        return  C( *this ) *= r;
    }

    auto                        operator/( co C& r ) co
    {
        return  C( *this ) /= r;
    }
    ///-------------------------+---------------------------------------------------------------

    auto&                       operator+=( co T& r )
    {
        return  ( this->x() += r, this->y() += r ), *this;
    }

    auto&                       operator-=( co T& r )
    {
        return  ( this->x() -= r, this->y() -= r ), *this;
    }

    auto&                       operator*=( co T& r )
    {
        return  ( this->x() *= r, this->y() *= r ), *this;
    }

    auto&                       operator/=( co T& r )
    {
        return  ( this->x() /= r, this->y() /= r ), *this;
    }
    ///-------------------------+---------------------------------------------------------------

    auto                        operator+( co T& r ) co
    {
        return  C{ this->x() + r, this->y() + r };
    }

    auto                        operator-( co T& r ) co
    {
        return  C{ this->x() - r, this->y() - r };
    }

    auto                        operator*( co T& r ) co
    {
        return  C{ this->x() * r, this->y() * r };
    }

    auto                        operator/( co T& r ) co
    {
        return  C{ this->x() / r, this->y() / r };
    }
    ///-------------------------+---------------------------------------------------------------

    auto                        operator-() co
    {
        return  C{ -this->x(), -this->y() };
    }

    auto                        swap() co
    {
        return  C{ y(), x() };
    }

    auto                        round() co
    {
        return  C{ std::round( x() ), std::round( y() ) };
    }
    ///-------------------------+---------------------------------------------------------------

    auto                        yx() co
    {
        return  swap();
    }

    auto                        cross( co C& r ) co
    {
        return  x() * r.y() - y() * r.x();
    }

    auto                        dot( co C& r ) co                                           
    {
        return  x() * r.x() + y() * r.y();
    }

    auto                        square() co                                                     
    {
        return  x() * x() + y() * y();
    }

    auto                        length() co                                                     
    {
        return  std::hypot( x(), y() );
    }

    auto&                       normalize() co                                                  
    {
        if( length() )
            *this /= length();
        return  *this;
    }

    auto                        get_roll( co AB<f64>& r ) co                                        
    {
        return  AB<f64>{ cross( r ), dot( r ) };
    }

    auto&                       roll( co AB<f64>& r )                                       
    {
        return  *this = get_roll( r );
    }
};
//------------------------------+---------------------------------------------------------------
};

//------------------------------+---------------------------------------------------------------
//  ImGui adpator

#define  IM_VEC2_CLASS_EXTRA    ImVec2( co crux::XY<crux::f32>& p )\
    :   x( (float)p.x() ), y( (float)p.y() ) {}

//==============================================================================================
