#ifndef EXT_ENUMSET_HPP
#define EXT_ENUMSET_HPP

#include <bitset>
#include "strtok.h"
#include "convert.h"
#include "strenum.hpp"

//enum class flag
//  {
//  deleted,
//  virt,
//  hidden,
//  extended,
//  ncount,
//  badval = ncount
//  };
//
//ext::strenum<flag>::pair str_flag[] =
//  {
//  { flag::deleted,  "flag_deleted" },
//  { flag::virtual,  "flag_virtual" },
//  { flag::hidden,   "flag_hidden"  },
//  { flag::extended, "flag_extended" },
//  }
//
//STRENUM_INIT_VALUES( flag, str_flag, flag::badval );
//
//ext::enumset<flag,flag::ncount> m_flags;
//              ^           ^
//              |           |
//              |           +- bitset size
//              +- enum type
//
//std::string str = ext::covert_to<std::string>( m_flags ); // "flag_deleted,flag_virtual,flag_hidden,flag_extended"
//
//m_flags = ext::convert_to<decltype(m_flags)>( std::string("flag_deleted,flag_hidden") );
//

namespace ext
{

template<typename T, const T N>
class enumset:
    public std::bitset<static_cast<size_t>(N)>
{
public:
    typedef std::bitset<static_cast<size_t>(N)> base_type;

    typedef typename base_type::reference reference;
#if 0
    using base_type::operator [];
    using base_type::count;
    using base_type::size;
    using base_type::any;
    using base_type::none;
    using base_type::all;

    using base_type::flip;

    using base_type::to_string;
    using base_type::to_ulong;
    using base_type::to_ullong;

    using base_type::operator &=;
    using base_type::operator |=;
    using base_type::operator ^=;
    using base_type::operator <<=;
    using base_type::operator >>=;
    using base_type::operator ~;
    using base_type::operator <<;
    using base_type::operator >>;
    using base_type::operator ==;
    using base_type::operator !=;
#endif

    inline                  enumset(): base_type() {}
    inline                  enumset(const enumset<T,N>& rvalue): base_type( rvalue ) {}
    inline                  enumset(const std::string& str_vals)
        {
        for (const std::string& str_val : ext::strtok<>( ',', str_vals ))
            {
            size_t enum_val = static_cast<size_t>( ext::strenum<T>::get( str_val ) );
            if ( enum_val < static_cast<size_t>( N ) )
                base_type::set( enum_val );
            }
        }

    inline                  operator std::string () const
        {
        std::string result;
        for (size_t i = 0; i < static_cast<size_t>( N ); ++i) //test each bit
            {
            if ( !base_type::test( i ) )
                continue;

            const std::string& bitstr = ext::strenum<T>::get( static_cast<T>( i ) );
            if ( bitstr.empty() ) //can't resolve enum value
                continue;

            if ( result.empty() ) //first element
                result = bitstr;
            else
                {
                result += ',';
                result += bitstr;
                }
            }
        return result;
        }


    inline bool             test(const T pos) const
        {
        return base_type::test( static_cast<size_t>( pos ) );
        }
    inline enumset<T,N>&    set()
        {
        base_type::set();
        return *this;
        }
    inline enumset<T,N>&    set(const T pos, bool val = true)
        {
        base_type::set( static_cast<size_t>( pos ), val );
        return *this;
        }

    inline enumset<T,N>&    reset()
        {
        base_type::reset();
        return *this;
        }
    inline enumset<T,N>&    reset(const T pos)
        {
        base_type::reset( static_cast<size_t>( pos ) );
        return *this;
        }

    inline enumset<T,N>&    flip()
        {
        base_type::flip();
        return *this;
        }
    inline enumset<T,N>&    flip(const T pos)
        {
        base_type::flip( static_cast<size_t>( pos ) );
        return *this;
        }

    inline bool             operator [] (const T pos) const
        {
        return test( pos );
        }

    inline reference        operator [] (const T pos)
        {
        return base_type::operator [] ( static_cast<size_t>( pos ) );
        }

#ifdef EXT_OBSTREAM_HPP
    inline ext::obstream& operator >> (ext::obstream& obs) const
        {
        return obs << static_cast<const base_type&>( *this );
        }
#endif
#ifdef EXT_IBSTREAM_HPP
    inline ext::ibstream& operator << (ext::ibstream& ibs)
        {
        return ibs >> static_cast<base_type&>( *this );
        }
#endif
};

} //namespace ext

#endif

