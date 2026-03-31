#pragma once
//==============================================================================================
//  strings.h                   strings [**...]
//  CRUXELL.RND.software		jin lee
//  2018.02.20					2018.02.20
//------------------------------+---------------------------------------------------------------
#include <ctype.h>

namespace                       crux
{
//------------------------------+---------------------------------------------------------------

enum    class                   intex : u32
{
    hex,    dec,    bin,
};

enum    class                   alignment
{
    left,   center, right,
};

ttti                    auto    to_bin_string( T val )
{
    return  std::bitset<bit_sizeof( val )>( ( integral_t<T>& )val ).to_string();
}

//------------------------------+---------------------------------------------------------------

#pragma warning( disable : 4739 )
tt< intex X = intex::dec, tn T >
inl auto                        to_string( co T val )
{
    static_assert( sizeof( T ) <= sizeof( u64 ), "() bigger than 64bits" );

    if( X == intex::bin )
        return  to_bin_string( val );

    char    temp[ 256 ];

    cex  char*   formats[]
    {
        "0x%02X",   "0x%04X",   "0x%08X",   "0x%016llX",
        "%u",       "%u",       "%u",       "%llu",
        "%d",       "%d",       "%d",       "%lld",
    };
    const   uu  index = static_cast<u32>( X ) *
        ( std::is_signed_v<T> +1 ) * 4 + clog2( sizeof val );
    if( sizeof val < 4 )
    {
        u32 value = 0;
        *(T*)&value = (T)val;
        sprintf( temp, formats[ index ], value );
    }
    else
        sprintf( temp, formats[ index ], val );
    return  STR( temp );
}
///-----------------------------+---------------------------------------------------------------

ttt auto                        to_string( co T val, co intex X )
{
    if( X == intex::bin )    return  to_string<intex::bin>( val );
    else if( X == intex::dec )    return  to_string<intex::dec>( val );
    return  to_string<intex::hex>( val );
}
//------------------------------+---------------------------------------------------------------

tt< intex X = intex::dec, tn IT >
auto                            to_string( co IT begin, co IT end, co char* wrap = "[]" )
{
    STR out{ wrap[ 0 ] };
    for( auto& v : sub_<IT>( begin, end ) )
        out += STR( " " ) + to_string<X>( v );
    return  out += STR( " " ) + wrap[ 1 ];
}
//------------------------------+---------------------------------------------------------------
class                           WSTRING : public WSTR
{
	static              auto    isnt_space(co wchar_t c)
	{
		return  !isspace(c);
	}

	auto                        after_spaces()
	{
		return  std::find_if(begin(), end(), isnt_space);
	}

	auto                        before_spaces()
	{
		return  std::find_if(rbegin(), rend(), isnt_space).base();
	}

public:
	CRE                         WSTRING() = default;
	CRE                         WSTRING(co wchar_t* s) : WSTR(s) {}
	CRE                         WSTRING(co WSTR& str) : WSTR(str) {}
	CRE                         WSTRING(co uu size, co char c) : WSTR(size, c) {}

	CRE ttt                     WSTRING(co T begin, co T end) : WSTR(begin, end) {}
	CRE ttt                     WSTRING(co T value, co int digit = 3,
		sfifp(std::is_arithmetic_v<T>))
	{
		std::ostringstream  os;
		if (std::is_floating_point_v<T>)
			os << std::fixed << std::setprecision(digit);
		os << value;
		assign(os.str());
	}

	CRE                         WSTRING(co WSTR& s, int byte, co alignment al = alignment::right)
	{
		reserve(byte);

		if (al == alignment::center)
			resize((byte - s.length()) / 2, ' ');
		else if (al == alignment::right)
			resize(byte - s.length(), ' ');

		*this += s;
		if (size() != (size_t)byte)
			resize(byte, ' ');
	}

	auto&                       align(int byte, co alignment al = alignment::right)
	{
		WSTRING  s(*this, byte, al);
		s.swap(*this);
		return  *this;
	}

	auto                        f32() co { return  std::stof(*this); }
	auto                        f64() co { return  std::stod(*this); }
	auto                        i32() co { return  std::stoi(*this); }
	auto                        i64() co { return  std::stoll(*this); }

	auto                        pos(co WSTR s) co { return  (int)find(s); }

	ttt auto&                   sum(co T value)
	{
		return  *this += WSTRING(value);
	}

	tt< tn T, tn...Ts > auto&   sum(T value, Ts...vs)
	{
		return  sum(value), sum(vs...);
	}

	auto&                       ltrim()
	{
		return  assign(WSTR(after_spaces(), end()));
	}

	auto&                       rtrim()
	{
		return  assign(WSTR(begin(), before_spaces()));
	}

	auto&                       trim()
	{
		auto    l = after_spaces();
		auto    r = before_spaces();
		if (l > r)
			l = r;
		return  assign(std::wstring(l, r));
	}

	auto                        lsplit(WSTR&& s, co bool exclude_delimiter = true) co
	{
		auto    p = pos(s);
		if (p < 0)
			return  std::make_pair(*this, WSTRING());
		auto    m = begin() + p + !exclude_delimiter * s.size();
		return  std::make_pair(WSTRING(begin(), m),
			WSTRING(m + exclude_delimiter * s.size(), end()));
	}

	auto                        rsplit(WSTR&& s, co bool exclude_delimiter = true) co
	{
		auto    p = (int)rfind(s);
		if (p < 0)
			return  std::make_pair(WSTRING(), *this);
		auto    m = begin() + p + !exclude_delimiter * s.size();
		return  std::make_pair(WSTRING(begin(), m),
			WSTRING(m + exclude_delimiter * s.size(), end()));
	}

	auto                        sub(int offset, int size)
	{
		return  WSTRING(substr(offset, size));
	}
};
class                           STRING : public STR
{
    static              auto    isnt_space( co char c )
    {
        return  !isspace( c );
    }

    auto                        after_spaces()
    {
        return  std::find_if( begin(), end(), isnt_space );
    }

    auto                        before_spaces()
    {
        return  std::find_if( rbegin(), rend(), isnt_space ).base();
    }

public:
    CRE                         STRING() = default;
    CRE                         STRING( co char* s ) : STR( s ) {}
    CRE                         STRING( co STR& str ) : STR( str ) {}
    CRE                         STRING( co uu size, co char c ) : STR( size, c ) {}

    CRE ttt                     STRING( co T begin, co T end ) : STR( begin, end ) {}
    CRE ttt                     STRING( co T value, co int digit = 3,
                                        sfifp( std::is_arithmetic_v<T> ) )
    {
        std::ostringstream  os;
        if( std::is_floating_point_v<T> )
            os << std::fixed << std::setprecision( digit );
        os << value;
        assign( os.str() );
    }

    CRE                         STRING( co STR& s, int byte, co alignment al = alignment::right )
    {
        reserve( byte );

        if( al == alignment::center )
            resize( ( byte - s.length() ) / 2, ' ' );
        else if( al == alignment::right )
            resize( byte - s.length(), ' ' );

        *this += s;
        if( size() != (size_t)byte )
            resize( byte, ' ' );
    }

    auto&                       align( int byte, co alignment al = alignment::right )
    {
        STRING  s( *this, byte, al );
        s.swap( *this );
        return  *this;
    }

    auto                        f32() co { return  std::stof( *this ); }
    auto                        f64() co { return  std::stod( *this ); }
    auto                        i32() co { return  std::stoi( *this ); }
    auto                        i64() co { return  std::stoll( *this ); }

    auto                        pos( co STR s ) co { return  (int)find( s ); }

    ttt auto&                   sum( co T value )
    {
        return  *this += STRING( value );
    }

    tt< tn T, tn...Ts > auto&   sum( T value, Ts...vs )
    {
        return  sum( value ), sum( vs... );
    }

    auto&                       ltrim()
    {
        return  assign( STR( after_spaces(), end() ) );
    }

    auto&                       rtrim()
    {
        return  assign( STR( begin(), before_spaces() ) );
    }

    auto&                       trim()
    {
        auto    l = after_spaces();
        auto    r = before_spaces();
        if( l > r )
            l = r;
        return  assign( std::string( l, r ) );
    }

    auto                        lsplit( STR&& s, co bool exclude_delimiter = true ) co
    {
        auto    p = pos( s );
        if( p < 0 )
            return  std::make_pair( *this, STRING() );
        auto    m = begin() + p + !exclude_delimiter * s.size();
        return  std::make_pair( STRING( begin(), m ),
                                STRING( m + exclude_delimiter * s.size(), end() ) );
    }

    auto                        rsplit( STR&& s, co bool exclude_delimiter = true ) co
    {
        auto    p = (int)rfind( s );
        if( p < 0 )
            return  std::make_pair( STRING(), *this );
        auto    m = begin() + p + !exclude_delimiter * s.size();
        return  std::make_pair( STRING( begin(), m ),
                                STRING( m + exclude_delimiter * s.size(), end() ) );
    }

    auto                        sub( int offset, int size )
    {
        return  STRING( substr( offset, size ) );
    }
};
//------------------------------+---------------------------------------------------------------

class                           STRINGS : public std::list<STRING>
{
public:
    auto&                       open( co STRING filename, co bool skip_lastline = false )
    {
        FILE*   fp  = fopen( filename.c_str(), "rb" );
        int line_count = 0;
        resize( 1 );
        int c;
        while( ( c = fgetc( fp ) ) != EOF )
        {
            if( c == '\r' );
            else if( c == '\n' )
            {
                emplace_back( STR() );
                line_count++;
            }
            else back().push_back( (char)c );
        }
        if( skip_lastline )
            resize( line_count );
        fclose( fp );
        return  *this;
    }

    CRE                         STRINGS() = default;
    CRE ttt                     STRINGS( std::initializer_list<T> list ) { *this += list; }
    CRE                         STRINGS( co STRING filename, co bool skip_lastline = false )
    {
        open( filename, skip_lastline );
    }

    auto                        operator[]( co i32 index ) co
    {
        auto    it = begin();
        times( index ) ++it;
        return  *it;
    }

    auto&                       add( co STRING v )
    {
        emplace_back( v );
        return  *this;
    }

    auto                        str() co
    {
        STRING  s;
        for( auto& v : *this ) s += v + '\n';
        return  s;
    }

    auto                        save( co STRING filename )
    {
        FILE*   fp = fopen( filename.c_str(), "wb" );
        auto    s = str();
        fwrite( s.data(), s.size(), 1, fp );
        fclose( fp );
        return  *this;
    }

};
//------------------------------+---------------------------------------------------------------

tt< tn T, uu size > class       vector_array
{
public:
    using   list_type           = LIST<T>;
    using   series_type         = ARR<list_type,size>;
    using   iter_type           = tn list_type::iterator;

    series_type                 series;

    ///-------------------------+---------------------------------------------------------------
    class                       iterator
    {
        series_type&            series;
        iter_type               it;
        uu                      index;

    public:

        CRE                     iterator( series_type& series, u32 index, iter_type it )
        :   series( series )
        ,   index( index )
        ,   it( it )
        {}

        iterator&               operator++()
        {
            ++it;
            while( it == series[ index ].end() && ++index < array_size )
                it = series[ index ].begin();
            return  *this;
        }

        bool                    operator!=( iterator& other ) co
        {
            return  &*it != &*other;
        }

        auto                    operator*() co
        {
            return  *it;
        }
    };
    ///-------------------------+---------------------------------------------------------------

    auto                        begin() co
    {
        for( u32 i = 0; i < series.size(); ++i )
            if( series[ i ].size() )
                return  iterator( series, i, series[ i ].begin() );
        return  end();
    }

    auto                        end() co
    {
        return  iterator( series, array_size - 1, series[ array_size - 1 ].end() );
    }

    auto                        add( co uu index, co T& item )
    {
        series[ index ].emplace_back( item );
    }

    auto                        append( co uu index, co T* begin, co T* end )
    {
        todo( "implement" );
    }
};
//==============================================================================================
};
