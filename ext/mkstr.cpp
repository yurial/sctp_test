#include "mkstr.hpp"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

namespace ext
{

std::string mkstr(const char* fmt, ...)
    {
    char* str = NULL;
    int size = 0;

    va_list ap;
    va_start( ap, fmt );
    size = ::vasprintf( &str, fmt, ap );
    va_end( ap );

    if ( -1 == size ) //vasnprintf fail if memory allocation failed
        throw std::bad_alloc();

    std::string result( str, size );
    ::free( str );
    return result;
    }

// optimized appending of formatted string to an existing std::string
// this function intentionally returns void, to avoid mess with mkstr()
void append_mkstr(std::string& dst, const char* fmt, ...)
    {
    char* str = NULL;
    int size = 0;

    va_list ap;
    va_start( ap, fmt );
    size = ::vasprintf( &str, fmt, ap );
    va_end( ap );

    if ( -1 == size ) //vasnprintf fail if memory allocation failed
        throw std::bad_alloc();

    dst.append(str, size);
    ::free( str );
    }

} //namespace ext

