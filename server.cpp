#include <sys/timerfd.h>
#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <fcntl.h>

#include <iostream>

#include <ext/convert.hpp>
#include <ext/strenum.hpp>
#include <unistd/fd.hpp>
#include <unistd/netdb.hpp>
#include <unistd/epoll.hpp>
#include <unistd/time.hpp>

#include "receiver.hpp"

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

uint64_t counter_recv_requests = 0;
uint64_t counter_recv_messages = 0;
uint64_t counter_loops = 0;

int help(FILE* os, int argc, char* argv[])
    {
    fprintf( os, "usage: %s -h [hostname] [port]\n", argv[0] );
    fprintf( os, "  --help          help :)\n" );
    fprintf( os, "  --nodelay       disable Nagle algorithm\n" );
    fprintf( os, "  --sndbuf[k|m|g] send buffer\n" );
    fprintf( os, "  --rcvbuf[k|m|g] recv buffer\n" );
    return EXIT_SUCCESS;
    }

const char short_options[] = "h";

struct opt
    {
    enum enum_t
        {
        help = 'h',
        nodelay = 127,
        sndbuf,
        rcvbuf
        };
    };

static const option long_options[] =
    {
    { "help",       no_argument,        nullptr, opt::help      },
    { "nodelay",    no_argument,        nullptr, opt::nodelay   },
    { "sndbuf",     required_argument,  nullptr, opt::sndbuf    },
    { "rcvbuf",     required_argument,  nullptr, opt::rcvbuf    }
    };

struct params
    {
    bool        nodelay = false;
    std::string hostname = "localhost";
    std::string port = "31337";
    size_t      sndbuf = 0;
    size_t      rcvbuf = 0;
    };

params get_params(int argc, char* argv[])
    {
    params p;
    optind = 1;
    opterr = 0;
    int option_index;
    for (;;)
        {
        int option = getopt_long( argc, argv, short_options, long_options, &option_index );
        if ( -1 == option )
            break;
        switch( option )
            {
            case opt::help:
                help( stdout, argc, argv );
                exit( EXIT_SUCCESS );
            case opt::nodelay:
                p.nodelay = true;
                break;
            case opt::sndbuf:
                p.sndbuf = ext::convert_to<bytes,std::string>( optarg );
                break;
            case opt::rcvbuf:
                p.rcvbuf = ext::convert_to<bytes,std::string>( optarg );
                break;
#if 0
            case opt_file_limit:
                p.file_limit = ext::convert_to<bytes,std::string>( optarg );
                break;
            case opt_page_size:
                p.page_size = ext::convert_to<bytes,std::string>( optarg );
                if ( p.page_size > std::numeric_limits<int32_t>::max() )
                    {
                    syslog( LOG_ERR ) << "page size is too big" << std::endl;
                    exit( EXIT_FAILURE );
                    }
                break;
#endif
            case 0:
                fprintf( stderr, "Invalid command-line option\n" );
                help( stderr, argc, argv );
                exit( EXIT_FAILURE );
            }
        }
    if ( optind < argc )
        p.hostname = argv[ optind++ ];
    if ( optind < argc )
        p.port = argv[ optind ];

    return p;
    }

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

void process_message(const mmsghdr& message)
{
const msghdr& hdr = message.msg_hdr;
size_t nrecv = message.msg_len;

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
    ++counter_recv_requests;
}

void process_messages(const receiver::result& messages)
{
std::for_each( messages.begin(), messages.end(), process_message );
}

void show_stat()
{
fprintf( stdout, "loops: %lu\n", counter_loops );
fprintf( stdout, "msg per recv: %lu\n", counter_recv_messages / counter_loops );
fprintf( stdout, "rps: %lu\n", counter_recv_requests );
fprintf( stdout, "\n" );

counter_loops = 0;
counter_recv_messages = 0;
counter_recv_requests = 0;
}

int main(int argc, char* argv[])
{
signal( SIGTRAP, SIG_IGN );
params p = get_params( argc, argv );
unistd::addrinfo hint = addrinfo{ AI_PASSIVE, AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, 0, nullptr, nullptr, nullptr };
std::vector<unistd::addrinfo> addrs = unistd::getaddrinfo( p.hostname, p.port, hint );
const unistd::addrinfo& addr = addrs.at( 0 );
unistd::fd sock = unistd::socket( addr );

int flags = fcntl( sock, F_GETFL, 0 );
fcntl( sock, F_SETFL, flags | O_NONBLOCK );

if ( 0 != p.sndbuf )
    unistd::setsockopt( sock, SOL_SOCKET, SO_SNDBUF, p.sndbuf );

if ( 0 != p.rcvbuf )
    unistd::setsockopt( sock, SOL_SOCKET, SO_RCVBUF, p.rcvbuf );

if ( p.nodelay )
    unistd::setsockopt( sock, SOL_SCTP, SCTP_NODELAY, 1 );

struct sctp_sack_info sack_info{ 0, 1, 1 };
unistd::setsockopt( sock, SOL_SCTP, SCTP_DELAYED_SACK, sack_info );

//TODO: unistd::sctp::bindx
unistd::bind( sock, addr );
unistd::listen( sock, 128 );
subscribe_events( sock );

unistd::fd timerfd = std::move( timerfd_create( CLOCK_MONOTONIC, TFD_NONBLOCK ) );
itimerspec timeout{ { 1, 0 }, { 1, 0 } };
timerfd_settime( timerfd, 0, &timeout, nullptr );
unistd::fd efd = std::move( unistd::epoll_create() );
unistd::epoll_add( efd, sock, EPOLLIN, static_cast<int>( sock ) );
unistd::epoll_add( efd, timerfd, EPOLLIN, static_cast<int>( timerfd ) );

receiver rcv( 8192, 1, CMSG_SPACE( sizeof( sctp_sndrcvinfo ) ), 1024 );

const unistd::timespec loop_time( 0, 1000000L ); //1ms
const unistd::timespec minimal_sleep_time( 0, 1000L ); //1mcs

for (;;)
    {
    ++counter_loops;
    const std::vector<epoll_event> events = unistd::epoll_wait( efd, 4/*maxevents*/, -1/*timeout*/ );
    const unistd::timespec start_time = unistd::clock_gettime( CLOCK_MONOTONIC );
    for (const auto& event : events)
        {
        if ( event.data.fd == sock )
            {
            if ( event.events & EPOLLIN )
                {
                const receiver::result messages = rcv.recv( sock );
                counter_recv_messages += messages.size();
                process_messages( messages );
                }
            }
        else if ( event.data.fd == timerfd )
            {
            uint64_t x;
            unistd::read_all( timerfd, &x, sizeof(x) );
            show_stat();
            }
        else
            raise( SIGTRAP );
        }
    const unistd::timespec end_time = unistd::clock_gettime( CLOCK_MONOTONIC );
    const unistd::timespec elapse_time = end_time - start_time;
    const unistd::timespec sleep_time = loop_time - elapse_time;
    if ( sleep_time > minimal_sleep_time )
        unistd::nanosleep( sleep_time );
    }

#if 0
sendmsg
#endif
//raise( SIGTRAP );

sock.close();
return EXIT_SUCCESS;
}

