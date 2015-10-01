#ifndef EXT_STRENUM_HPP
#define EXT_STRENUM_HPP

#include <typeinfo>
#include <stdexcept>
#include <tr1/unordered_map>

#include "mkstr.hpp"

//enum myenum { MY_ENUM_VALUE, BAD_VALUE };
//ext::strenum<myenum>::pair str_val[] = { MY_ENUM_VALUE, "string value" };
//STRENUM_INIT( myenum, str_val, BAD_VALUE )
// --
//ext::strenum<myenum>::get( "my string" );
//ext::strenum<myenum>::get( MY_ENUM_VALUE );

#define STRENUM_INIT_VALUES(T,V,BADVAL)\
    template <>\
    const T ext::strenum<T>::enum_not_found = BADVAL;\
    template <>\
    const std::string ext::strenum<T>::str_not_found = std::string();\
    template <>\
    const typename ext::strenum<T>::enum_to_str_type ext::strenum<T>::m_enum_to_str = ext::strenum<T>::generate_enum_to_str( sizeof(V)/sizeof(V[0]), V );\
    template <>\
    const typename ext::strenum<T>::str_to_enum_type ext::strenum<T>::m_str_to_enum = ext::strenum<T>::generate_str_to_enum( sizeof(V)/sizeof(V[0]), V );

#define STRENUM_CONVERT_TO(T)\
    namespace ext\
    {\
    template <typename O, typename I>\
    inline  O convert_to(const I& value);\
    \
    template<> inline T convert_to(const std::string& strval) { return strenum<T>::get( strval ); }\
    template<> inline T convert_to(char* const& strval) { return strenum<T>::get( strval ); }\
    template<> inline T convert_to(const char* const& strval) { return strenum<T>::get( strval ); }\
    template<> inline std::string convert_to(const T& enumval) { return strenum<T>::get( enumval ); }\
    } //namespace ext

namespace ext
{

template <typename T>
class strenum
    {
    public:
        typedef T                       enum_type;
        struct pair
            {
            enum_type   key;
            const char* val;
            };

    protected:
        struct hash
            {
            inline size_t               operator() (enum_type val) const;
            }; //struct hash

        typedef std::tr1::unordered_map<enum_type,std::string,hash> enum_to_str_type;
        typedef std::tr1::unordered_map<std::string,enum_type>      str_to_enum_type;

    public:
        static  enum_to_str_type        generate_enum_to_str(size_t count, const pair values[]);
        static  str_to_enum_type        generate_str_to_enum(size_t count, const pair values[]);

        static  const std::string&      get(enum_type key);
        static  enum_type               get(const std::string& val);

    protected:
        static  const enum_type         enum_not_found;
        static  const std::string       str_not_found;
        static  const enum_to_str_type  m_enum_to_str;
        static  const str_to_enum_type  m_str_to_enum;
    }; //class strenum

template <typename T>
size_t strenum<T>::hash::operator() (enum_type val) const
    {
    return std::tr1::hash<int>()( static_cast<int>( val ) );
    }

template <typename T>
typename strenum<T>::enum_to_str_type strenum<T>::generate_enum_to_str(size_t count, const pair values[])
    {
    enum_to_str_type result;
    for (size_t i = 0; i < count; ++i)
        {
        std::pair<typename enum_to_str_type::iterator,bool> pair = result.insert( typename enum_to_str_type::value_type( values[ i ].key, values[ i ].val ) );
        if ( !pair.second )
            throw std::logic_error( ext::mkstr( "strenum<%s>: duplicate value: %d=%s=%s", typeid(enum_type).name(), values[ i ].key, pair.first->second.c_str(), values[ i ].val ) );
        }
    return result;
    }

template <typename T>
typename strenum<T>::str_to_enum_type strenum<T>::generate_str_to_enum(size_t count, const pair values[])
    {
    str_to_enum_type result;
    for (size_t i = 0; i < count; ++i)
        {
        std::pair<typename str_to_enum_type::iterator,bool> pair = result.insert( typename str_to_enum_type::value_type( values[ i ].val, values[ i ].key ) );
        if ( !pair.second )
            throw std::logic_error( ext::mkstr( "strenum<%s>: duplicate key: %s=%d=%d", typeid(enum_type).name(), values[ i ].val, pair.first->second, values[ i ].key ) );
        }
    return result;
    }

template <typename T>
const std::string& strenum<T>::get(enum_type key)
    {
    typename enum_to_str_type::const_iterator it = m_enum_to_str.find( key );
    if ( m_enum_to_str.end() == it )
        return strenum<T>::str_not_found;
    return it->second;
    }

template <typename T>
T strenum<T>::get(const std::string& val)
    {
    typename str_to_enum_type::const_iterator it = m_str_to_enum.find( val );
    if ( m_str_to_enum.end() == it )
        return strenum<T>::enum_not_found;
    return it->second;
    }

} //namespace ext

#endif

