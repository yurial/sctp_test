#ifndef EXT_MKSTR_HPP
#define EXT_MKSTR_HPP

#include <string>

namespace ext
{

//make std::string using vasptrinf
std::string mkstr(const char* fmt, ...);

// optimized appending of formatted string to an existing std::string
void append_mkstr(std::string& dst, const char* fmt, ...);

} //namespace ext

#endif

