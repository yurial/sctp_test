#ifndef EXT_CONVERT_HPP
#define EXT_CONVERT_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <algorithm>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

class bytes
{
protected:
uint64_t    m_value;

public:
inline  bytes(uint64_t value): m_value( value ) {}
inline operator uint64_t () const { return m_value; }
};

class base26:
    public std::string
    {
    public:
        inline  base26(const std::string str):
            std::string( str )
            {}
    };

class base64:
    public std::string
    {
    public:
        inline  base64(const std::string str):
            std::string( str )
            {}
    };

class json;

namespace ext
{

template <typename O, typename I>
inline O convert_to(const I& value)
    {
    return O( value );
    }

template <>
inline int16_t convert_to(char* const& value) //FIXME DEPRECATED
    {
    return atol( value );
    }

template <>
inline uint16_t convert_to(char* const& value) //FIXME DEPRECATED
    {
    return atol( value );
    }


template <>
inline int32_t convert_to(char* const& value) //FIXME DEPRECATED
    {
    return atol( value );
    }

template <>
inline uint32_t convert_to(char* const& value) //FIXME DEPRECATED
    {
    return atol( value );
    }

template <>
inline int64_t convert_to(char* const& value) //FIXME DEPRECATED
    {
    return atoll( value );
    }

template <>
inline uint64_t convert_to(char* const& value) //FIXME DEPRECATED
    {
    return atoll( value );
    }

template <>
inline int16_t convert_to(const std::string& value)
    {
    return atol( value.c_str() );
    }

template <>
inline uint16_t convert_to(const std::string& value)
    {
    return atol( value.c_str() );
    }

template <>
inline int32_t convert_to(const std::string& value)
    {
    return atol( value.c_str() );
    }

template <>
inline uint32_t convert_to(const std::string& value)
    {
    return atol( value.c_str() );
    }


template <>
inline int64_t convert_to(const std::string& value)
    {
    return atoll( value.c_str() );
    }

template <>
inline uint64_t convert_to(const std::string& value)
    {
    return atoll( value.c_str() );
    }

template <>
inline int16_t convert_to(const char* const& value)
    {
    return atol( value );
    }

template <>
inline uint16_t convert_to(const char* const& value)
    {
    return atol( value );
    }

template <>
inline int32_t convert_to(const char* const& value)
    {
    return atol( value );
    }

template <>
inline uint32_t convert_to(const char* const& value)
    {
    return atol( value );
    }

template <>
inline int64_t convert_to(const char* const& value)
    {
    return atoll( value );
    }

template <>
inline uint64_t convert_to(const char* const& value)
    {
    return atoll( value );
    }

template <>
inline bool convert_to(const char* const& value)
    {
    return convert_to<int64_t>( value ) != 0;
    }

template <>
inline bool convert_to(char* const& value)
    {
    return convert_to<int64_t>( value ) != 0;
    }

template <>
inline std::string convert_to(char* const& value)
    {
    return std::string(value);
    }

template <>
inline std::string convert_to(const bool& value)
    {
    return value ? "1" : "0";
    }

template <>
inline std::string convert_to(const int16_t& value)
    {
    // boost::lexical cast is fastest, faster than std::to_string()
    return ::boost::lexical_cast<std::string>(value);
    }

template <>
inline std::string convert_to(const uint16_t& value)
    {
    // boost::lexical cast is fastest, faster than std::to_string()
    return ::boost::lexical_cast<std::string>(value);
    }

template <>
inline std::string convert_to(const int32_t& value)
    {
    // boost::lexical cast is fastest, faster than std::to_string()
    return ::boost::lexical_cast<std::string>(value);
    }

template <>
inline std::string convert_to(const uint32_t& value)
    {
    // boost::lexical cast is fastest, faster than std::to_string()
    return ::boost::lexical_cast<std::string>(value);
    }

template <>
inline std::string convert_to(const int64_t& value)
    {
    // boost::lexical cast is fastest, faster than std::to_string()
    return ::boost::lexical_cast<std::string>(value);
    }

template <>
inline std::string convert_to(const uint64_t& value)
    {
    // boost::lexical cast is fastest, faster than std::to_string()
    return ::boost::lexical_cast<std::string>(value);
    }

template <>
inline std::string convert_to(const in_addr& addr)
    {
    char ip[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &(addr.s_addr), ip, INET_ADDRSTRLEN );
    return ip;
    }

template <>
inline std::string convert_to(const double& value)
    {
#define BUFF_SIZE 1024
    char buff[BUFF_SIZE];
    snprintf( buff, BUFF_SIZE, "%lf", value );
#undef BUFF_SIZE
    return buff;
    }

template <>
inline float convert_to( const std::string& value)
    {
    return atof( value.c_str() );
    }

template <>
inline base26 convert_to(const uint64_t& v)
    {
    static const int base = 26;

    uint64_t value = v;
    std::string result;
    result.reserve(16);
    do
        {
        div_t divres = div(value, base);
        result += 'a' + divres.rem;

        value = divres.quot;
        }
    while (value != 0);

    std::reverse(result.begin(),result.end());
    return result;
    }

template <>
inline base26 convert_to(const int64_t& v)
    {
    return convert_to<base26,uint64_t>(v);
    }

template <>
inline base26 convert_to(const uint32_t& v)
    {
    return convert_to<base26,uint64_t>(v);
    }

template <>
inline base26 convert_to(const int32_t& v)
    {
    return convert_to<base26,uint64_t>(v);
    }

template <>
inline base26 convert_to(const uint8_t& v)
    {
    return convert_to<base26,uint64_t>(v);
    }

template <>
inline base26 convert_to(const char& v)
    {
    return convert_to<base26,uint8_t>(v);
    }

#if GCC_VERSION > 40700
template <>
inline uint64_t convert_to(const base26& value)
    {
    if (value[0] >= '0' && value[0] <= '9')
        return convert_to<uint64_t>(value.c_str());

    static const int base = 26;
    uint64_t result = 0;
    uint64_t order  = 1;

    for(base26::const_reverse_iterator i = value.crbegin(); i != value.crend(); ++i)
        {
        result += order * ((*i) - 'a');
        order  *= base;
        }
    return result;
    }
#else
// old GCC
template <>
inline uint64_t convert_to(const base26& v)
    {
    base26 value = v;
    if (value[0] >= '0' && value[0] <= '9')
        return convert_to<uint64_t>(value.c_str());

    std::reverse(value.begin(),value.end());
    static const int base = 26;
    uint64_t result = 0;
    uint64_t order  = 1;

    for(base26::const_iterator i = value.begin(); i != value.end(); ++i)
        {
        result += order * ((*i) - 'a');
        order  *= base;
        }
    return result;
    }
#endif

template <>
inline uint32_t convert_to(const base26& v)
    {
    return convert_to<uint64_t>(v);
    }

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

template <>
inline base64 convert_to(const std::string& input)
    {
    const char* bytes_to_encode = input.c_str();
    std::size_t in_len = input.length();
    std::string result;
    result.reserve( in_len * 1.4 );
    std::size_t i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--)
        {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
            {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                result += base64_chars[char_array_4[i]];
            i = 0;
            }
        }

    if (i)
        {
        std::size_t j = 0;
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            result += base64_chars[char_array_4[j]];

        while((i++ < 3))
            result += '=';
        }
    return result;
    }

template <>
inline std::string convert_to(const base64& encoded_string)
    {
    std::size_t in_len = encoded_string.size();
    std::size_t i = 0;
    std::size_t in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string result;
    result.reserve(in_len);

    while (in_len-- && ( encoded_string[in_] != '='))
        {
        char_array_4[i++] = encoded_string[in_++];
        if (i ==4)
            {
            for (i = 0; i <4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                result += char_array_3[i];
            i = 0;
            }
        }
    if (i)
        {
        std::size_t j = 0;
        for (j = i; j <4; j++)
            char_array_4[j] = 0;

        for (j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++)
            result += char_array_3[j];
        }
    return result;
    }

template <>
inline bytes convert_to(const std::string& str)
    {
    char* end = nullptr;
    unsigned long long value = strtoll( str.c_str(), &end, 10 );
    switch ( *end )
        {
        case 't':
        case 'T':
            value *= 1024;
        case 'g':
        case 'G':
            value *= 1024;
        case 'm':
        case 'M':
            value *= 1024;
        case 'k':
        case 'K':
            value *= 1024;
        case 0:
            break;
        default:
            throw std::out_of_range( "can't convert string to bytes" );
        }
    return bytes( value );
    }

} //namespace ext

#endif

