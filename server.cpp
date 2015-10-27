#include <sys/timerfd.h>
#include <sys/mman.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <fcntl.h>

#include <iostream>
#include <memory>
#include <atomic>

#include <ext/convert.hpp>
#include <ext/strenum.hpp>
#include <unistd/fd.hpp>
#include <unistd/netdb.hpp>
#include <unistd/epoll.hpp>
#include <unistd/time.hpp>

#include "receiver.hpp"
#include "fork.hpp"

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

struct counters
    {
    std::atomic<uint64_t>   recv_requests;
    std::atomic<uint64_t>   recv_messages;
    std::atomic<uint64_t>   recv_calls;
    std::atomic<uint64_t>   loops;
        counters():
                recv_requests( 0 ),
                recv_messages( 0 ),
                recv_calls( 0 ),
                loops( 0 )
            {
            }
    };

std::unique_ptr<counters> g_counters( new counters );
size_t g_worker_id = 0;

int help(FILE* os, int argc, char* argv[])
    {
    fprintf( os, "usage: %s -h [hostname] [port]\n", argv[0] );
    fprintf( os, "  --help          help :)\n" );
    fprintf( os, "  --workers       count of workers\n" );
    fprintf( os, "  --nodelay       disable Nagle algorithm\n" );
    fprintf( os, "  --sndbuf[k|m|g] send buffer\n" );
    fprintf( os, "  --rcvbuf[k|m|g] recv buffer\n" );
    fprintf( os, "  --loops         loops count per second, 0 = disable (default = 1000)\n");
    fprintf( os, "  --batch         count of messages per recvmmsg (default = 1024)\n");
    fprintf( os, "  --sack-freq     SACK frequency\n" );
    fprintf( os, "  --sack-timeout  SACK timeout\n" );
    fprintf( os, "  --peeloff       branch off an association into a separate socket\n" );
    fprintf( os, "  --oneshot       use epoll ONESHOT flag to serialize access to socket\n" );
    fprintf( os, "  --buf-size      buffer size for each message\n" );
    return EXIT_SUCCESS;
    }

const char short_options[] = "hw:";

struct opt
    {
    enum enum_t
        {
        help = 'h',
        workers = 'w',
        nodelay = 127,
        sndbuf,
        rcvbuf,
        loops,
        batch,
        sack_freq,
        sack_timeout,
        peeloff,
        oneshot,
        bufsize
        };
    };

static const option long_options[] =
    {
    { "help",           no_argument,        nullptr, opt::help          },
    { "workers",        required_argument,  nullptr, opt::workers       },
    { "nodelay",        no_argument,        nullptr, opt::nodelay       },
    { "sndbuf",         required_argument,  nullptr, opt::sndbuf        },
    { "rcvbuf",         required_argument,  nullptr, opt::rcvbuf        },
    { "loops",          required_argument,  nullptr, opt::loops         },
    { "batch",          required_argument,  nullptr, opt::batch         },
    { "sack-freq",      required_argument,  nullptr, opt::sack_freq     },
    { "sack-timeout",   required_argument,  nullptr, opt::sack_timeout  },
    { "peeloff",        no_argument,        nullptr, opt::peeloff       },
    { "oneshot",        no_argument,        nullptr, opt::oneshot       },
    { "bufsize",        required_argument,  nullptr, opt::bufsize       },
    { nullptr,          no_argument,        nullptr, 0                  }
    };

struct params
    {
    size_t      workers = 1;
    bool        nodelay = false;
    std::string hostname = "localhost";
    std::string port = "31337";
    size_t      sndbuf = 0;
    size_t      rcvbuf = 0;
    uint32_t    loops = 1000;
    size_t      batch = 1024;
    uint32_t    sack_freq = 0;
    uint32_t    sack_timeout = 0;
    bool        peeloff = false;
    bool        oneshot = false;
    size_t      bufsize = (64*1024);
    };

params g_params;

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
            case opt::workers:
                p.workers = ext::convert_to<decltype(p.workers)>( optarg );
                break;
            case opt::nodelay:
                p.nodelay = true;
                break;
            case opt::sndbuf:
                p.sndbuf = ext::convert_to<bytes,std::string>( optarg );
                break;
            case opt::rcvbuf:
                p.rcvbuf = ext::convert_to<bytes,std::string>( optarg );
                break;
            case opt::loops:
                p.loops = ext::convert_to<decltype(p.loops)>( optarg );
                break;
            case opt::batch:
                p.batch = ext::convert_to<decltype(p.batch)>( optarg );
                break;
            case opt::sack_freq:
                p.sack_freq = ext::convert_to<decltype(p.sack_freq)>( optarg );
                break;
            case opt::sack_timeout:
                p.sack_timeout = ext::convert_to<decltype(p.sack_timeout)>( optarg );
                break;
            case opt::peeloff:
                p.peeloff = true;
                break;
            case opt::oneshot:
                p.oneshot = true;
                break;
            case opt::bufsize:
                p.bufsize = ext::convert_to<bytes,std::string>( optarg );
                break;
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

int get_epoll_flags()
    {
    return g_params.oneshot? EPOLLONESHOT | EPOLLIN : EPOLLIN;
    }

void notification_handle(int efd, int sock, const sctp_notification& notify)
{
switch ( notify.sn_header.sn_type )
    {
    case SCTP_ASSOC_CHANGE:
        {
        const auto& sac = notify.sn_assoc_change;
std::cout << "^^^ assoc_change: state=" << ext::convert_to<std::string>( static_cast<sctp_sac_state>( sac.sac_state ) ) << " assoc_id=" << sac.sac_assoc_id << " error=" << sac.sac_error << " in=" << sac.sac_inbound_streams << " out=" << sac.sac_outbound_streams << std::endl;
        if ( g_params.peeloff )
            {
            switch ( sac.sac_state )
                {
                case SCTP_COMM_UP:
                    {
                    int fd = sctp_peeloff( sock, sac.sac_assoc_id );
                    if ( -1 != fd )
                        unistd::epoll_add( efd, fd, get_epoll_flags(), fd );
                    break;
                    }
                case SCTP_SHUTDOWN_COMP:
                    //unistd::epoll_del( efd, sock );  //TODO: close socket
                    break;
                default:
                    break;
                }
            }
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

void process_message(int efd, int sock, const mmsghdr& message)
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
    notification_handle( efd, sock, *reinterpret_cast<sctp_notification*>( hdr.msg_iov[0].iov_base ) );
else
    ++g_counters->recv_requests;
}

void process_messages(int efd, int sock, const receiver::result& messages)
{
for (const auto& message : messages)
    process_message( efd, sock, message );
}

void show_stat()
{
fprintf( stdout, "worker_id: %zu\n", g_worker_id );
fprintf( stdout, "loops: %lu\n", g_counters->loops.load() );
fprintf( stdout, "msg per recv: %0.2lf\n", g_counters->recv_calls.load()? static_cast<double>(g_counters->recv_messages.load()) / static_cast<double>(g_counters->recv_calls.load()) : 0 );
fprintf( stdout, "rps: %lu\n", g_counters->recv_requests.load() );
fprintf( stdout, "\n" );

g_counters->loops = 0;
g_counters->recv_calls = 0;
g_counters->recv_messages = 0;
g_counters->recv_requests = 0;
}

int main(int argc, char* argv[])
{
signal( SIGTRAP, SIG_IGN );
void* shared_buf = ::mmap( nullptr, sizeof(counters), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0 );
g_counters.reset( new( shared_buf ) counters() );

g_params = get_params( argc, argv );

unistd::fd efd = std::move( unistd::epoll_create() );

unistd::addrinfo hint = addrinfo{ AI_PASSIVE, AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, 0, nullptr, nullptr, nullptr };
std::vector<unistd::addrinfo> addrs = unistd::getaddrinfo( g_params.hostname, g_params.port, hint );
const unistd::addrinfo& addr = addrs.at( 0 );
unistd::fd sock = unistd::socket( addr );

int flags = fcntl( sock, F_GETFL, 0 );
fcntl( sock, F_SETFL, flags | O_NONBLOCK );

if ( 0 != g_params.sndbuf )
    unistd::setsockopt( sock, SOL_SOCKET, SO_SNDBUF, g_params.sndbuf );

if ( 0 != g_params.rcvbuf )
    unistd::setsockopt( sock, SOL_SOCKET, SO_RCVBUF, g_params.rcvbuf );

if ( g_params.nodelay )
    unistd::setsockopt( sock, SOL_SCTP, SCTP_NODELAY, 1 );

if ( g_params.sack_freq || g_params.sack_timeout )
    {
    const struct sctp_sack_info sack_info{ 0, g_params.sack_timeout, g_params.sack_freq };
    unistd::setsockopt( sock, SOL_SCTP, SCTP_DELAYED_SACK, sack_info );
    }

//TODO: unistd::sctp::bindx
unistd::bind( sock, addr );
unistd::listen( sock, 128 );
subscribe_events( sock );
unistd::epoll_add( efd, sock, get_epoll_flags(), static_cast<int>( sock ) );

unistd::fd timerfd = std::move( timerfd_create( CLOCK_MONOTONIC, TFD_NONBLOCK ) );
itimerspec timeout{ { 1, 0 }, { 1, 0 } };
timerfd_settime( timerfd, 0, &timeout, nullptr );
unistd::epoll_add( efd, timerfd, EPOLLONESHOT | EPOLLIN, static_cast<int>( timerfd ) );

receiver rcv( g_params.bufsize, 1, CMSG_SPACE( sizeof( sctp_sndrcvinfo ) ), g_params.batch );

for (size_t i = 1; i < g_params.workers; ++i)
    {
    if( 0 == fork( CLONE_FILES | SIGCHLD ) )
        {
        g_worker_id = i;
        break;
        }
    }

const unistd::timespec minimal_sleep_time( 0, 1000L ); //1mcs
const unistd::timespec start_time = unistd::clock_gettime( CLOCK_MONOTONIC );
for (uint64_t i = 1;; ++i)
    {
    const std::vector<epoll_event> events = unistd::epoll_wait( efd, 1/*maxevents*/, -1/*timeout*/ );
    const auto& event = events[ 0 ];
    if ( event.data.fd == timerfd )
        {
        uint64_t x = 0;
        unistd::read_all( timerfd, &x, sizeof(x) );
        unistd::epoll_mod( efd, timerfd, EPOLLONESHOT | EPOLLIN, timerfd );
        show_stat();
        continue;
        }
    else
        {
        if ( event.events & EPOLLIN )
            {
            ++g_counters->recv_calls;
            int fd = event.data.fd;
            //fprintf( stderr, "worker_id: %zu recv msg from %d\n", g_worker_id, fd );
            const receiver::result messages = rcv.recv( fd );
            //fprintf( stderr, "worker_id: %zu recv %zu msg from %d\n", g_worker_id, messages.size(), fd );
            if ( g_params.oneshot )
                unistd::epoll_mod( efd, fd, get_epoll_flags(), fd );
            g_counters->recv_messages += messages.size();
            process_messages( efd, sock, messages );
            }
        }

    ++g_counters->loops;
    if ( g_params.loops )
        {
        const uint64_t estimated_shift = static_cast<double>( i ) * 1000000000.0 / static_cast<double>( g_params.loops );
        const unistd::timespec estimated_end = start_time + unistd::timespec( estimated_shift / 1000000000L, estimated_shift % 1000000000L );
        const unistd::timespec end_time = unistd::clock_gettime( CLOCK_MONOTONIC );
        const unistd::timespec sleep_time = estimated_end - end_time;
        if ( sleep_time > minimal_sleep_time )
            unistd::nanosleep( sleep_time );
        else if ( sleep_time < unistd::timespec( 0, 0 ) )
            {
            //fix 'i'
            const unistd::timespec elapse_time = end_time - start_time;
            i = (static_cast<double>(elapse_time.tv_sec) + static_cast<double>(elapse_time.tv_nsec) / 1000000000.0) * static_cast<double>( g_params.loops );
            }
        }
    }

#if 0
sendmsg
#endif
//raise( SIGTRAP );

sock.close();
return EXIT_SUCCESS;
}

