#include <signal.h>
#include <stdlib.h>

#include <unistd/fd.hpp>
#include <unistd/netdb.hpp>
#include <netinet/sctp.h>

int main()
{
signal( SIGTRAP, SIG_IGN );
unistd::addrinfo hint = addrinfo{ 0, AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, 0, nullptr, nullptr, nullptr };
std::vector<unistd::addrinfo> addrs = unistd::getaddrinfo( "localhost", "31337", hint );
const unistd::addrinfo& addr = addrs.at( 0 );
unistd::fd sock = unistd::socket( addr );

    std::vector<char> msg_buff( 8192 ); //TODO:

    struct iovec iov;
    memset( &iov, 0, sizeof(iov) );
    iov.iov_base = msg_buff.data();
    iov.iov_len = msg_buff.size();

    struct msghdr hdr;
    memset( &hdr, 0, sizeof(hdr) );
    hdr.msg_name = const_cast<sockaddr_storage*>( &addr.ai_addr );
    hdr.msg_namelen = addr.ai_addrlen;
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;
    hdr.msg_flags = 0;

    ssize_t nrecv = 0;
    TEMP_FAILURE_RETRY( nrecv = sendmsg( sock, &hdr, 0 ) );
    if ( 0 == nrecv )
        raise( SIGTRAP );
    if ( -1 == nrecv )
        raise( SIGTRAP );
    raise( SIGTRAP );

#if 0
//TODO: handle packets
sendmsg
recvmsg
#endif
//raise( SIGTRAP );

sock.close();
return EXIT_SUCCESS;
}

