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

receiver::result receiver::recv(int fd, timespec* timeout)
    {
    int nrecv = ::recvmmsg( fd, m_mhdrs.data(), m_mhdrs.size(), 0 /*MSG_WAITFORONE*/, timeout );
    if ( -1 == nrecv )
        {
        if ( EAGAIN == errno )
            nrecv = 0;
        else
            throw std::system_error( errno, std::system_category(), "recvmmsg" );
        }
    return result( m_mhdrs.begin(), m_mhdrs.begin() + nrecv, *this );
    }

