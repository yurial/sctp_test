#include <algorithm>
#include <system_error>

#include "receiver.hpp"

receiver::receiver(size_t buf_size, int buf_count, size_t cmsg_size, int n):
        m_cmsg_size( cmsg_size )
    {
    init( buf_size, buf_count, cmsg_size, n );
    }

void receiver::init(size_t buf_size, int buf_count, size_t cmsg_size, int n)
    {
    const mmsghdr zero_hdr{ { nullptr, 0, nullptr, 0, nullptr, 0, 0 }, 0 };

    m_cmsg_buf.resize( cmsg_size * n );
    m_msg_buf.resize( buf_size * buf_count * n );
    m_iovs.resize( n * buf_count );
    m_mhdrs.resize( n );
    std::fill( m_mhdrs.begin(), m_mhdrs.end(), zero_hdr );
    for (size_t i = 0; i < m_iovs.size(); ++i)
        {
        m_iovs[ i ].iov_base = m_msg_buf.data() + ( i * buf_size );
        m_iovs[ i ].iov_len = buf_size;
        }
    for (int i = 0; i < n; ++i)
        {
        auto& hdr = m_mhdrs[ i ].msg_hdr;
        hdr.msg_iov = &m_iovs[ i * buf_count ];
        hdr.msg_iovlen = buf_count;
        hdr.msg_control = m_cmsg_buf.data() + ( i * cmsg_size );
        hdr.msg_controllen = cmsg_size;
        hdr.msg_flags = 0;
        }
    }

void receiver::cleanup(std::vector<mmsghdr>::iterator begin, std::vector<mmsghdr>::iterator end)
    {
    std::for_each( begin, end, [this](mmsghdr& mhdr) { mhdr.msg_hdr.msg_controllen = m_cmsg_size; } );
    }

receiver::result receiver::recv(int fd)
{
if ( m_mhdrs.size() > 1 )
    return recvmmsg( fd );
else if ( m_mhdrs.size() == 1 )
    return recvmsg( fd );
return result( m_mhdrs.end(), m_mhdrs.end(), *this );
}

receiver::result receiver::recvmsg(int fd)
{
ssize_t nrecv = ::recvmsg( fd, &m_mhdrs[0].msg_hdr, 0 /*flags*/ );
if ( -1 == nrecv )
    {
    if ( EAGAIN == errno )
        nrecv = 0;
    else
        throw std::system_error( errno, std::system_category(), "recvmsg" );
    }
if ( 0 == nrecv )
    return result( m_mhdrs.end(), m_mhdrs.end(), *this );

m_mhdrs[0].msg_len = nrecv;
return result( m_mhdrs.begin(), m_mhdrs.begin() + 1, *this );
}

receiver::result receiver::recvmmsg(int fd)
    {
    int nrecv = ::recvmmsg( fd, m_mhdrs.data(), m_mhdrs.size(), 0 /*MSG_WAITFORONE*/, nullptr );
    if ( -1 == nrecv )
        {
        if ( EAGAIN == errno )
            nrecv = 0;
        else
            throw std::system_error( errno, std::system_category(), "recvmmsg" );
        }
    return result( m_mhdrs.begin(), m_mhdrs.begin() + nrecv, *this );
    }

