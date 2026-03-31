#pragma once
//==============================================================================================
//  base.h                      core base [****.]
//  CRUXELL.RND.software		jin lee
//  2017.08.04					2018.03.10
//------------------------------+---------------------------------------------------------------
//  system classifications
#include <array>
#include <list>

#if     _MSC_VER && !__INTEL_COMPILER

#pragma warning(disable: 4996)
#pragma warning(disable: 4554)

#define MSVC

#define align64( definition )   __declspec( align( 64 ) ) definition
#include <intrin.h>

#else
#define align64( definition )   definition __attribute__((aligned( 64 )))
#include <x86intrin.h>
#endif

#if     ( _WIN64 || __x86_64__ || __ppc64__ )
#define SYS64
#else
#define SYS32
#endif

#ifdef  max
#undef  min
#undef  max
#endif
//------------------------------+---------------------------------------------------------------
//  affix

#define CRE
#define KIL
#define BITS
#define BYTES
#define __

//------------------------------+---------------------------------------------------------------
//  eaters

#define TODO( ... )             // TO DO comment ( basic requirements )
#define todo( ... )             // to do comment ( advanced requirements )
#define tip( ... )              // hint

#define NOTICE( ... )
#define DANGER( ... )

//------------------------------+---------------------------------------------------------------
//  coloring

#define crux                    crux

//------------------------------+---------------------------------------------------------------
//  typeless consts

#define nil                     nullptr
#define null                    (char)0
#define byte_bits               8
#define msb_mask_byte           0x80

#define sys_bytes               sizeof( std::size_t )
#define bit_sizeof( v )         ( sizeof( v ) * byte_bits )
#define sys_bits                ( sys_bytes * byte_bits )

#define sse_pack_bytes          ( 128 BITS  / byte_bits )
#define is_32bit                ( sys_bits == 32 BITS )

#define is_numeric( c )         ( (c) <= '9'  && (c) >= '0' )
#define is_lower( c )           ( (c) <= 'z'  && (c) >= 'a' )
#define is_upper( c )           ( (c) <= 'Z'  && (c) >= 'A' )
#define is_alpha( c )           ( is_lower(c) || is_upper(c) )

#define is_name( c )            ( (c) <= 'z'  && (c) >= '0' ) && \
                                ( (c) <= '9'  || (c) >= 'a' || is_upper(c) || (c) == '_' )

//------------------------------+---------------------------------------------------------------
//  concepts

#define co                      const
#define cex                     constexpr
#define naming                  using   namespace
#define countof( arr )          ( sizeof(arr) / sizeof(arr[0]) )

#define fn                      class
#define tn                      typename
#define inl                     inline

#define tt                      template
#define ttt                     template< typename T >
#define ttti                    template< typename T > inline

#define fun( F )                const F&
#define dm( T, S )              typename T, std::size_t S
#define con( C, T )             template< class, class > class C, class T                       //  template parameter ( container )
#define conp( C, T )            C<T,std::allocator<T>>                                          //  template function parameter ( container )

namespace                       crux
{
    //------------------------------+---------------------------------------------------------------
    //  types

    using   i8 = char;
    using   i16 = short;
    using   i32 = int;
    using   i64 = long long;

    using   u8 = unsigned char;
    using   u16 = unsigned short;
    using   u32 = unsigned int;
    using   u64 = unsigned long long;

    using   uu = std::size_t;
    using   ii = std::make_signed_t<uu>;
    using   w32 = unsigned long;                                                //  DWORD

    using   f32 = float;
    using   f64 = double;

    using   STR = std::string;

    ///-----------------------------+---------------------------------------------------------------

    ttt                     using   P = T * ;
    ttt                     using   LIST = std::vector<T>;
    tt< tn T, uu S >        using   arr = T(&)[S];                                            //  C language array
    tt< tn T, uu S >        using   ARR = std::array<T, S>;

    ///-----------------------------+---------------------------------------------------------------

    tt< ii >                struct  to {};
    tt< ii...ns >           struct  seq {};

    tt< ii from = 0 >       struct  seq_up
    {
        tt< ii n, ii...ns > struct  end : end< n - 1, n - 1, ns... > {};
        tt< ii...ns >       struct  end< from, ns... > : seq< ns... > {};
        tt< ii last >       struct  to : end< last + 1 > {};
    };

    tt< tn T, uu SA, ii...A, uu SB, ii...B >
        cex auto                        subcat(ARR<T, SA> a, seq<A...>, ARR<T, SB> b, seq<B...>)
    {
        return  ARR<T, sizeof...(A) + sizeof...(B)>{ std::get<A>(a)..., std::get<B>(b)... };
    }

#define aindex( a )             a, seq_up<0>::end<countof(a)>{}

    tt< tn T, uu SA, uu SB >
        cex auto                        concat(ARR<T, SA> a1, ARR<T, SB> a2)
    {
        return  subcat(aindex(a1), aindex(a2));
    }
    ///-----------------------------+---------------------------------------------------------------

    tt< dm(T, S), ii index >
        cex auto                        partial_sum(ARR<T, S> nums, to<index>)
    {
        return  std::get<index>(nums) + partial_sum(nums, to<index - 1>{});
    }

    tt< dm(T, S) >
        cex auto                        partial_sum(ARR<T, S> nums, to<0>)
    {
        return  std::get<0>(nums);
    }
    //------------------------------+---------------------------------------------------------------
    //  "ranged for" support

    ttt                     class   sub_
    {
        co  T                       org, fin;
    public:
        CRE     sub_(co T org, co T fin) : org(org), fin(fin) {}
        auto    begin() co { return  org; }
        auto    end() co { return  fin; }
    };

    ttti                    auto    range_cut(T& list, co int head = 0, co int rear = 0)
    {
        return  sub_<decltype(std::begin(list))>(
            std::begin(list) + head, std::end(list) + rear);
    }

    ttti                    auto    range_sub(T& list, co int head = 0, co int size = 0)
    {
        return  sub_<decltype(std::begin(list))>(std::begin(list) + head,
            size == 0 ? std::end(list) : std::begin(list) + head + size);
    }

    ttti                    auto    range_cut(std::list<T>& list, co int head = 0, co int rear = 0)
    {
        auto    h = list.begin(), t = list.end();
        for (int i = 0; i < head; ++i) ++h;
        for (int i = 0; i > rear; --i) --t;
        return  sub_<decltype(h)>(h, t);
    }

#define soul( i_, v_, ... )     (;0;); int i_=v_-1; for( __VA_ARGS__ ) if( ++i_,1 )             //  outer scoped counter
#define tzuyu( i_, v_, ... )    ( int i_=v_-1, r_=1; r_ ; r_=0 ) for( __VA_ARGS__ ) if( ++i_,1 )//  inner scoped counter
#define love( ... )             tzuyu( i, 0, __VA_ARGS__ )                                      //  inner scoped counter named i

#define walk( from, to, steps ) \
    ( int run = 1; run; ) for( const auto f = from; run; run ^= 1 ) \
        for( auto i_ = decltype(from)(); ( from = pick( f, to, i_, steps ) ) <= to; ++i_ )

    ///-----------------------------+---------------------------------------------------------------

#define times( n )              for( auto i = (decltype(n))0; i < n; ++i )
#define fo( a, ... )            for( auto a = __VA_ARGS__ > a; ++a )

    ttt                     auto    extend_sides(T& list, co int window)
    {
        co  auto    filler0 = list[window];
        fo(x, 0; window) list[x] = filler0;
        co  auto    filler1 = list[list.size() - window - 1];
        fo(x, list.size() - window; list.size()) list[x] = filler1;
    }
    //------------------------------+---------------------------------------------------------------
    //  unrollers

    tt< ii N, fn F > inl  auto      unroll(to<N>, fun(F) f)
    {
        unroll(to<N - 1>{}, f);
        f(N);
    }

    tt<       fn F > inl  auto      unroll(to<0>, fun(F) f) { f(0); }

    //------------------------------+---------------------------------------------------------------
    //  SFINAE

#define sfsize( b )				ttt cex bool is##b = sizeof( T ) == b
#define sfif( cond, RT )		tn std::enable_if<cond,RT>                                      //  template<class T>             sfif( is8<T>, void ) func( T ) { ... }
#define sfifp( ... )            bool(*)[ __VA_ARGS__ ] = nil                                    //  template<class T>             auto func( T v, sfifp( sizeof v == 8 ) ) { ... }

#define if_( cond )             std::conditional_t<cond,
#define then_( match )          match,
#define else_( other )          other>

    tt< tn...Ts >           struct  are_same;
    tt< tn A >              struct  are_same<A, A> { enum { result = true }; };
    tt< tn A, tn B >        struct  are_same<A, B> { enum { result = false }; };
    tt< tn A, tn B, tn...rest >
        struct  are_same<A, B, rest...> {
        enum {
            result =
            are_same<A, B>::result && are_same<B, rest...>::result
        };
    };

    sfsize(8); sfsize(4); sfsize(2); sfsize(1);

    ///-----------------------------+---------------------------------------------------------------

    ttt                     using   integral_t = if_(is8<T>) then_(i64)                 //  support arithmetic bitwise even if the operand is a float type
        else_(if_(is4<T>) then_(i32)
            else_(if_(is2<T>) then_(i16)
                else_(i8)));

    ttt                     using   bitwise_t = std::make_unsigned_t<integral_t<T>>;              //  support logical bitwise even if the operand is a float type

    ttt cex                 auto    msb_mask()
    {
        return  bitwise_t<T>(msb_mask_byte << ((sizeof(T) - 1) * byte_bits));
    }
#define msb_mask( v )           msb_mask<decltype(v)>()
    ttt cex                 auto    msb(T value) { return  (T)(msb_mask(value) & value); }

    //------------------------------+---------------------------------------------------------------
    //  casters

    ttt                     using   type_ = std::underlying_type_t<T>;
    ttt                     using   valuetype = std::remove_pointer_t< std::remove_reference_t<
        std::remove_all_extents_t<T>>>;

    ttt cex                 auto    value(T e) { return  static_cast<type_<T>>(e); }      //  easy to use for enum class
    ttt cex tn      valuetype<T>    de_ref(T && t) { return  t; }

#define is_cex( c )             noexcept( de_ref( c ) )
#define tracktype( var )        valuetype<decltype( var )>
#define flattype( var )         std::remove_cv_t<tracktype( var )>
#define take( L, R )            L = (flattype(decltype(L))&)(R);

    //------------------------------+---------------------------------------------------------------
    //  evaluators

    tt< uu S = 8 > inl        auto    align(uu v)
    {
        cex uu  limit = byte - 1;
        return  v + limit & ~limit;
    }

    tt< uu S = 16, tn T > inl auto    align(co P<T> ptr)
    {
        return  (pV<T>)(align<S>((uu)ptr));
    }

#define round_up( x, s )        (((x)+((s)-1)) & -(s))

    ///-----------------------------+---------------------------------------------------------------

#define rating( real, prec )    std::ratio< ii( (real) * (prec) ), (prec) >, prec


    cex                     uu      clog2(co uu value, co uu count = 0)
    {
        return  value < 2 ? count : clog2(value >> 1, count + 1);
    }

    cex                     uu      cpow(co uu value, co uu count)
    {
        return  count > 1 ? value : cpow(value, count - 1) * value;
    }
    //------------------------------+---------------------------------------------------------------

    ttt inl i32                     qc(co T a, co T b)                                            //  quick compare : -1 0 1
    {
        return  -(a < b) | (a > b);
    }
    //------------------------------+---------------------------------------------------------------

    ttti                    auto    copy(P<T> d, P<co T> s, co uu size)
    {
        return  (P<T>)std::memcpy(d, s, sizeof *s * size);
    }

    tt< tn D, tn S >    inl auto    copy(P<D> d, P<co S> s, co uu size)
    {
        return  std::copy(s, s + size, d), d;
    }

    tt< tn D, tn S >    inl auto    copy(P<D> d, co S& c)
    {
        return  std::copy(c.begin(), c.end(), d), d;
    }
    ///-----------------------------+---------------------------------------------------------------

    ttti                    auto    fill(P<T> arr, co T& value, co uu size)
    {
        return  std::fill(arr, arr + size, value), arr;
    }

    tt< tn T, fn F >    inl auto    inplace(P<T> ptr, P<T> end, fun(F) unary)
    {
        return  std::for_each(std::begin(ptr), std::end(end), unary);
    }

    tt< tn D, tn S1, tn S2, fn F >
        inl                     auto    combine(P<D> dst, P<co S1> s1, P<co S1> s1e, P<co S2> s2,
            fun(F) binary)
    {
        return  std::transform(s1, s1e, s2, dst, binary);
    }
    ///-----------------------------+---------------------------------------------------------------

    tt< con(L, T), tn R >
        inl                     auto&   operator+=(conp(L, T)& total, std::initializer_list<R>&& some)
    {
        for (co auto item : some)
            total.emplace_back((T)item);
        return  total;
    }

    tt< con(L, T), tn R >
        inl                     auto&   operator+=(conp(L, T)& total, co R& some)
    {
        for (co auto item : some)
            total.emplace_back((T)item);
        return  total;
    }

    tt< con(L, T), dm(R, S) >
        inl                     auto&   operator+=(conp(L, T)& total, arr<R, S> some)
    {
        return  total += (ARR<R, S>&)*some;
    }
    ///-----------------------------+---------------------------------------------------------------

    tt< con(L, T), tn R >
        inl                     auto    operator+(conp(L, T)& total, co R& some)
    {
        auto    temp = total;
        return  temp += some;
    }

    tt< con(L, T), dm(R, S) >
        inl                     auto    operator+(conp(L, T)& total, arr<R, S> some)
    {
        auto    temp = total;
        return  temp += (ARR<R, S>&)*some;
    }

    ttt inl                 auto    operator<<(std::ostream& os, T&& some)
        -> decltype(some.str(), os)
    {
        return  os << some.str();
    }
    //==============================================================================================
};
