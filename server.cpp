#include <signal.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>

#include <iostream>

#include <ext/strenum.hpp>
#include <unistd/fd.hpp>
#include <unistd/netdb.hpp>

#if 0
    { SCTP_ADDR_AVAILABLE, "SCTP_ADDR_AVAILABLE" },
    { SCTP_ADDR_UNREACHABLE, "SCTP_ADDR_UNREACHABLE" },
    { SCTP_ADDR_REMOVED, "SCTP_ADDR_REMOVED" },
    { SCTP_ADDR_ADDED, "SCTP_ADDR_ADDED" },
    { SCTP_ADDR_MADE_PRIM, "SCTP_ADDR_MADE_PRIM" },
    { SCTP_ADDR_CONFIRMED, "SCTP_ADDR_CONFIRMED" }
#endif
ext::strenum<sctp_sac_state>::pair sctp_sac_state_str[] =
    {
    { SCTP_COMM_UP,         "SCTP_COMM_UP"          },
    { SCTP_COMM_LOST,       "SCTP_COMM_LOST"        },
    { SCTP_RESTART,         "SCTP_RESTART"          },
    { SCTP_SHUTDOWN_COMP,   "SCTP_SHUTDOWN_COMP"    },
    { SCTP_CANT_STR_ASSOC,  "SCTP_CANT_STR_ASSOC"   }
    };
STRENUM_INIT_VALUES( sctp_sac_state, sctp_sac_state_str, static_cast<sctp_sac_state>( -1 ) )
STRENUM_CONVERT_TO( sctp_sac_state )

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
std::cout << "^^^ assoc_change: state=" << ext::convert_to<std::string>( static_cast<sctp_sac_state>( sac.sac_state ) ) << " assoc_id=" << sac.sac_assoc_id << " error=" << sac.sac_error << " in=" << sac.sac_inbound_streams << " out=" << sac.sac_outbound_streams << std::endl;
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
        printf( "^^^ addr_change: %s state=%d, error=%d\n", ap, spc.spc_state, spc.spc_error );
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
        printf( "^^^ SCTP_SHUTDOWN_EVENT\n" );
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

void dump_packet(const msghdr& hdr, ssize_t nrecv)
{
static uint64_t packet_counter = 0;
for (cmsghdr* cmsg = CMSG_FIRSTHDR( &hdr ); cmsg != nullptr; cmsg = CMSG_NXTHDR( const_cast<msghdr*>( &hdr ), cmsg ))
    {
    if ( cmsg->cmsg_level == IPPROTO_SCTP )
        {
        switch ( cmsg->cmsg_type )
            {
            case SCTP_INIT:
                {
                const sctp_initmsg* cdata = reinterpret_cast<const sctp_initmsg*>( CMSG_DATA( cmsg ) );
                break;
                }
            case SCTP_SNDRCV:
                {
                const sctp_sndrcvinfo* cdata = reinterpret_cast<const sctp_sndrcvinfo*>( CMSG_DATA( cmsg ) );
                break;
                }
            default:
                raise( SIGTRAP );
            } //switch
        } //if
    } //for
if ( hdr.msg_flags & MSG_NOTIFICATION )
    notification_handle( *reinterpret_cast<sctp_notification*>( hdr.msg_iov[0].iov_base ) );
else
    ++packet_counter;
}

void recv_packets(int sock, int events)
{
std::vector<char> cmsg_buff( CMSG_SPACE( sizeof( sctp_sndrcvinfo ) ) );
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
dump_packet( hdr, nrecv );
}

void show_stat(int sock, int events)
{

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

unistd::fd efd = std::move( epoll_create1( 0 ) );
epoll_event event;
event.events = EPOLLIN | EPOLLOUT;
event.data.fd = static_cast<int>( sock );
epoll_ctl( efd, EPOLL_CTL_ADD, sock, &event );

for (;;)
    {
    std::vector<epoll_event> events( 4 ); //TODO: define
    int nready = epoll_wait( efd, events.data(), events.size(), -1 );
    for (int i = 0; i < nready; ++i)
        {
        if ( events[ i ].data.fd == sock )
            recv_packets( sock, events[ i ].events );
        else
            raise( SIGTRAP );
        }
    }

#if 0
sendmsg
#endif
//raise( SIGTRAP );

sock.close();
return EXIT_SUCCESS;
}

