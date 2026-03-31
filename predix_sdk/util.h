#pragma once
//==============================================================================================
//  util.h                      utility functions [**...]
//  CRUXELL.RND.software		jin lee
//  2017.08.04					2018.02.19
//------------------------------+---------------------------------------------------------------

#ifdef MSVC 

inl  void   say( co std::string str )
{
    CString strMsg((CA2T)str.c_str());
    MessageBox(NULL, strMsg, _T("say"), NULL);
}
#endif

#include <io.h>

#pragma warning(push)
#pragma warning(disable:4244)			// (변환하면서 데이터가 손실될 수 있습니다)

namespace                       crux
{
//------------------------------+---------------------------------------------------------------

ttt auto    print( co T* begin, co T* end, co char* wrap = "[]" )
{
    std::cout << to_string<>( begin, end, wrap );
}

ttt auto    print( co T* ptr, co uu size, co char* wrap = "[]" )
{
    std::cout << to_string<>( ptr, ptr + size, wrap );
}

tt< tn T, int size >
auto        print( co arr<T,size>& arr, co char* wrap = "[]" )
{
    std::cout << to_string<>( arr, arr + size, wrap );
}

tt< tn T, int size >
auto        print( co ARR<T, size>& arr, co char* wrap = "[]" )
{
    std::cout << to_string<>( arr.begin(), arr.end(), wrap );
}

tt< con(C,T) >
auto        print( co conp( C, T )& list, co char* wrap = "[]" )
{
    std::cout << to_string<>( list.begin(), list.end(), wrap );
}

#define println( ... )          { print( __VA_ARGS__ ); std::cout << '\n'; }

//------------------------------+---------------------------------------------------------------
inl  std::pair<WSTRING/*filepath*/, WSTRING/*ext*/> split_path_ext(co WSTRING fullpath)
{
	auto    r = fullpath.rsplit(L".");
	r.second = L"." + r.second;
	return  r;
}
inl  std::pair<STRING/*filepath*/,STRING/*ext*/> split_path_ext( co STRING fullpath )
{
    auto    r = fullpath.rsplit( "." );
    r.second = "." + r.second;
    return  r;
}

inl  std::pair<STRING/*path*/,STRING/*name*/> split_path_name( co STRING filepath )
{
    auto    r = filepath.rsplit( "\\" );
    r.first.push_back( '\\' );
    return  r;  
}

inl  std::pair<WSTRING/*path*/, WSTRING/*name*/> split_path_name(co WSTRING filepath)
{
	auto    r = filepath.rsplit(L"\\");
	r.first.push_back(L'\\');
	return  r;
}

inl  auto                       extract_filename( co STRING filepath )
{
    return  split_path_name( filepath ).second;
}
inl  auto                       extract_filename(co WSTRING filepath)
{
	return  split_path_name(filepath).second;
}

inl  auto                       extract_nameonly( co STRING filepath )
{
    return  split_path_name( split_path_ext( filepath ).first ).second;
}

inl  auto                       extract_fileext( co STRING filepath )
{
    return  split_path_ext( filepath ).second;
}
inl auto                        is_raw_data(STRING filename)
{
	std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

	return   !(split_path_ext(filename).second.compare(".raw")) | !(split_path_ext(filename).second.compare(".RAW"));
};
inl auto                        is_tiff_file(co wchar_t* filename)
{
	WSTRING filenamew = filename;
	std::transform(filenamew.begin(), filenamew.end(), filenamew.begin(), ::tolower);

	return   !(split_path_ext(filenamew).second.compare(L".tiff")) | !(split_path_ext(filenamew).second.compare(L".tif"));
};
inl auto                        is_crx_file(co wchar_t* filename)
{
	WSTRING filenamew = filename;
	std::transform(filenamew.begin(), filenamew.end(), filenamew.begin(), ::tolower);

	return   !(split_path_ext(filenamew).second.compare(L".crx")) | !(split_path_ext(filenamew).second.compare(L".CRX"));
};
inl auto                        is_crx_file(STRING filename)
{
	std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

	return   !(split_path_ext(filename).second.compare(".crx")) | !(split_path_ext(filename).second.compare(".CRX"));
};
inl auto                        is_tiff_file( STRING filename)
{
	std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

	return   !(split_path_ext(filename).second.compare(".tiff")) | !(split_path_ext(filename).second.compare(".tif"));
};

inl auto                        is_raw_file(STRING filename)
{
	std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
	return   !(split_path_ext(filename).second.compare(".raw")) ;
};

inl auto                        is_tiff_file(WSTRING filename)
{
	std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

	return   !(split_path_ext(filename).second.compare(L".tiff")) | !(split_path_ext(filename).second.compare(L".tif"));
};

inl auto                        is_raw_file(WSTRING filename)
{
	std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
	return   !(split_path_ext(filename).second.compare(L".raw"));
};

inl auto                        file_exist( co STRING& filename )
{
    return  std::ifstream( filename ).good();
};
//------------------------------+---------------------------------------------------------------

tt< intex X = intex::dec, int column = 16, tn IT >
auto                            to_array_string( co IT begin, co IT end, STRING name )
{
    STRING  out;
    using   type = decltype(*begin);
    out.sum( "const   ", typeid( type ).name(), " ", name, "[ ", end - begin, " ] = {" );
    if( end > begin )
    {
        int col = 0;
        for( auto& item : sub_<IT>( begin, end ) )
        {
            if( !col )
                out += "\n    ";
            out += to_string<X>( item ) + ", ";
            if( ++col == column )
                col = 0;
        }
    }
    out += "\n};";
    return  out;
}

tt< intex X = intex::hex, con( C, T ) >
auto                            dump( co STR filename, co conp( C, T )& con )
{
    std::ofstream   fs( filename );
    fs << to_array_string<X>( con.begin(), con.end(), extract_filename( filename ) );
    fs.close();
}
//------------------------------+---------------------------------------------------------------

inl char*                       convert( co wchar_t* src )
{
    static  char    dst[ 260 ];
    wcstombs( dst, src, sizeof dst );
    return  dst;
}
//------------------------------+---------------------------------------------------------------



class                           redirect
{
    FILE*                       fp;
    int                         fd;
    fpos_t                      pos;

public:

    CRE                         redirect( co char* filename, co char* mode = "wb",
                                          FILE* as = stdout )
    {
        fgetpos( as, &pos );
        fd = _dup( _fileno( as ) );
        fp = freopen( filename, mode, as );
    }

    KIL                        ~redirect()
    {
        fflush( fp );
        _dup2( fd, _fileno( fp ) );
        _close( fd );
        clearerr( fp );
        fsetpos( fp, &pos );
    }
};
//------------------------------+---------------------------------------------------------------

tt< dm( T, S ), ii i >
cex auto    julian_48months( ARR<T, S> nums, to<i>, int leap_start )
{
    return  i % 14 == 0 ? 0 : partial_sum( nums, to<i>{} ) + ( i >= leap_start );
}

tt< dm( T, S ), ii...i >
cex auto    acc_( ARR<T,S> nums, seq<i...>, ii leap )
{
    auto    a = concat( ARR<T,1>{ 0 }, nums );
    auto    s = concat( concat( a, a ), concat( a, a ) );
    return  ARR<T,sizeof...(i)>
        { julian_48months( s, to<i>{}, leap ? (S+1)*3+3 : sizeof...(i) )... };
}

tt< dm( T, S ) >
cex auto    acc( ARR<T,S> nums, bool leap = false )
{
    return  acc_( nums, seq_up<0>::end<(S+1)*4>{}, leap );
}

cex auto	days_months	        = ARR<int,13>{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, };
cex auto    julian_4years       = acc( days_months );
cex auto    julian_4years_leap  = acc( days_months, true );

//==============================================================================================
};

#pragma warning(pop)
