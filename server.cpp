#include <signal.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>

#include <unistd/fd.hpp>
#include <unistd/netdb.hpp>

void subscribe_events(int fd)
{
struct sctp_event_subscribe events;
memset( &events, 0, sizeof(events) );
events.sctp_data_io_event = 1;
events.sctp_association_event = 1;
events.sctp_address_event = 1;
events.sctp_send_failure_event = 1;
events.sctp_peer_error_event = 1;
events.sctp_shutdown_event = 1;
events.sctp_partial_delivery_event = 1;
events.sctp_adaptation_layer_event = 1;
events.sctp_authentication_event = 1;

unistd::setsockopt( fd, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events) );
}

void notification_handle(const sctp_notification& notify)
{
switch ( notify.sn_header.sn_type )
    {
    case SCTP_ASSOC_CHANGE:
        {
        const auto& sac = notify.sn_assoc_change;
        printf( "^^^ assoc_change: state=%hu, error=%hu, instr=%hu outstr=%hu\n", sac.sac_state, sac.sac_error, sac.sac_inbound_streams, sac.sac_outbound_streams );
        break;
        }
    case SCTP_SEND_FAILED:
        {
        const auto& ssf = notify.sn_send_failed;
        printf( "^^^ sendfailed: len=%hu err=%d\n", ssf.ssf_length, ssf.ssf_error );
        break;
        }
    case SCTP_PEER_ADDR_CHANGE:
        {
        const auto& spc = notify.sn_paddr_change;
        char addrbuf[INET6_ADDRSTRLEN];
        const char* ap = nullptr;
        if ( spc.spc_aaddr.ss_family == AF_INET)
            {
            const auto sin = reinterpret_cast<const struct sockaddr_in*>( &spc.spc_aaddr );
            ap = inet_ntop(AF_INET, &sin->sin_addr, addrbuf, INET6_ADDRSTRLEN);
            }
        else
            {
            const auto sin6 = reinterpret_cast<const struct sockaddr_in6*>( &spc.spc_aaddr );
            ap = inet_ntop(AF_INET6, &sin6->sin6_addr, addrbuf, INET6_ADDRSTRLEN);
            }
        printf( "^^^ intf_change: %s state=%d, error=%d\n", ap, spc.spc_state, spc.spc_error );
        break;
        }
    case SCTP_REMOTE_ERROR:
        {
        const auto& sre = notify.sn_remote_error;
        printf( "^^^ remote_error: err=%hu len=%hu\n", ntohs(sre.sre_error), ntohs(sre.sre_length) );
        break;
        }
    case SCTP_SHUTDOWN_EVENT:
        {
        printf( "^^^ shutdown event\n" );
        break;
        }
    default:
        {
        raise( SIGTRAP );
        printf( "unknown type: %hu\n", notify.sn_header.sn_type );
        break;
        }
    };
}

int main()
{
signal( SIGTRAP, SIG_IGN );
unistd::addrinfo hint = addrinfo{ AI_PASSIVE, AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, 0, nullptr, nullptr, nullptr };
std::vector<unistd::addrinfo> addrs = unistd::getaddrinfo( "localhost", "31337", hint );
const unistd::addrinfo& addr = addrs.at( 0 );
unistd::fd sock = unistd::socket( addr );
//TODO: unistd::sctp::bindx
unistd::bind( sock, addr );
unistd::listen( sock, 128 );
subscribe_events( sock );

for (;;)
    {
    std::vector<char> cmsg_buff( sizeof(struct cmsghdr) + sizeof(struct sctp_sndrcvinfo) );
    std::vector<char> msg_buff( 8192 ); //TODO:

    struct iovec iov;
    memset( &iov, 0, sizeof(iov) );
    iov.iov_base = msg_buff.data();
    iov.iov_len = msg_buff.size();

    struct msghdr hdr;
    memset( &hdr, 0, sizeof(hdr) );
    hdr.msg_name = nullptr;
    hdr.msg_namelen = 0;
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;
    hdr.msg_control = cmsg_buff.data();
    hdr.msg_controllen = cmsg_buff.size();
    hdr.msg_flags = 0;

    ssize_t nrecv = 0;
    TEMP_FAILURE_RETRY( nrecv = recvmsg( sock, &hdr, 0 ) );
    if ( 0 == nrecv )
        raise( SIGTRAP );
    if ( -1 == nrecv )
        raise( SIGTRAP );

    if ( hdr.msg_flags & MSG_NOTIFICATION )
        notification_handle( *reinterpret_cast<sctp_notification*>( msg_buff.data() ) );
    else
        raise( SIGTRAP );
    }
#if 0
for (cmsgptr = CMSG_FIRSTHDR(&msg); cmsgptr != NULL;
    cmsgptr = CMSG_NXTHDR(&msg, cmsgptr)) {
    if (cmsgptr->cmsg_level == ... && cmsgptr->cmsg_type == ... ) {
       u_char  *ptr;

       ptr = CMSG_DATA(cmsgptr);
       /* process data pointed to by ptr */
    }
sendmsg
#endif
//raise( SIGTRAP );

sock.close();
return EXIT_SUCCESS;
}

