#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include <arpa/inet.h>

#include <vector>

#include <ext/subvector.hpp>

class receiver
{
public:
class result:
        public ext::subvector<mmsghdr>
    {
    protected:
    receiver&   m_receiver;

    public:
    inline  result(std::vector<mmsghdr>::iterator b, std::vector<mmsghdr>::iterator e, receiver& r):
            ext::subvector<mmsghdr>( b, e ),
            m_receiver( r )
        {
        }
    inline  ~result()
        {
        m_receiver.cleanup( begin(), end() );
        }
    };
friend class result;

            receiver(size_t msg_size, int buf_n, size_t cmsg_size, int n);
result      recv(int fd);
protected:
size_t                  m_cmsg_size;
std::vector<char>       m_cmsg_buf;
std::vector<char>       m_msg_buf;
std::vector<iovec>      m_iovs;
std::vector<mmsghdr>    m_mhdrs;

void        init(size_t msg_size, int buf_n, size_t cmsg_size, int n);
void        cleanup(std::vector<mmsghdr>::iterator begin, std::vector<mmsghdr>::iterator end);

result      recvmsg(int fd);
result      recvmmsg(int fd);
}; //class receiver

#endif

