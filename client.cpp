#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>

#include <iostream>
#include <limits>

#include <ext/convert.hpp>
#include <unistd/fd.hpp>
#include <unistd/netdb.hpp>

#if 0
sctp_assocparams assoc_params;
assoc_params.sasoc_assoc_id
assoc_params.sasoc_asocmaxrxt
assoc_params.sasoc_number_peer_destinations
assoc_params.sasoc_peer_rwnd
assoc_params.sasoc_local_rwnd
assoc_params.sasoc_cookie_life
#endif

int help(FILE* os, int argc, char* argv[])
    {
    fprintf( os, "usage: %s [-h] [-n <count>] [hostname] [port]\n", argv[0] );
    fprintf( os, "  --help              help :)\n" );
    fprintf( os, "  --count             count of messages\n" );
    fprintf( os, "  --batch             count of messages per sendmmsg\n" );
    fprintf( os, "  --nodelay           disable Nagle algorithm\n" );
    fprintf( os, "  --sndbuf[k|m|g]     send buffer\n" );
    fprintf( os, "  --rcvbuf[k|m|g]     recv buffer\n" );
    fprintf( os, "  --mtu[k|m|g]        set MTU (ipv6 only)\n" );
    fprintf( os, "  --maxseg            (SCTP_MAXSEG) max packet size\n" );
    fprintf( os, "  --msgsize[k|m|g]    message size\n" );
    return EXIT_SUCCESS;
    }

const char short_options[] = "hn:";

struct opt
    {
    enum enum_t
        {
        help = 'h',
        count = 'n',
        nodelay = 127,
        batch,
        sndbuf,
        rcvbuf,
        max_burst,
        mtu,
        maxseg,
        msgsize
        };
    };

static const option long_options[] =
    {
    { "help",       no_argument,        nullptr, opt::help      },
    { "count",      required_argument,  nullptr, opt::count     },
    { "batch",      required_argument,  nullptr, opt::batch     },
    { "nodelay",    no_argument,        nullptr, opt::nodelay   },
    { "sndbuf",     required_argument,  nullptr, opt::sndbuf    },
    { "rcvbuf",     required_argument,  nullptr, opt::rcvbuf    },
    { "max-burst",  required_argument,  nullptr, opt::max_burst },
    { "mtu",        required_argument,  nullptr, opt::mtu       },
    { "maxseg",     required_argument,  nullptr, opt::maxseg    },
    { "msgsize",    required_argument,  nullptr, opt::msgsize   },
    { nullptr,      no_argument,        nullptr, 0              }
    };

struct params
    {
    uint64_t    count = std::numeric_limits<uint64_t>::max();
    bool        nodelay = false;
    std::string hostname = "localhost";
    std::string port = "31337";
    size_t      batch = 1024;
    int         sndbuf = 0;
    int         rcvbuf = 0;
    int         max_burst = 0;
    int         mtu = 0;
    int         maxseg = 0;
    size_t      msgsize = 8;
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
            case opt::count:
                p.count = ext::convert_to<decltype(p.count)>( optarg );
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
            case opt::max_burst:
                p.max_burst = ext::convert_to<decltype(p.max_burst)>( optarg );
                break;
            case opt::mtu:
                p.mtu = ext::convert_to<bytes,std::string>( optarg );
                break;
            case opt::maxseg:
                p.maxseg = ext::convert_to<bytes,std::string>( optarg );
                break;
            case opt::msgsize:
                p.msgsize = ext::convert_to<bytes,std::string>( optarg );
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
events.sctp_data_io_event = 0;
events.sctp_association_event = 1;
events.sctp_address_event = 0;
events.sctp_send_failure_event = 1;
events.sctp_peer_error_event = 1;
events.sctp_shutdown_event = 1;
events.sctp_partial_delivery_event = 0;
events.sctp_adaptation_layer_event = 0;
events.sctp_authentication_event = 0;

unistd::setsockopt( fd, SOL_SCTP, SCTP_EVENTS, &events, sizeof(events) );
}

std::vector<char> generate_message(size_t size)
{
std::vector<char> msg( size );
for (size_t i = 0; i < msg.size(); ++i)
    msg[ i ] = 'a' + i % 26;
return msg;
}

int main(int argc, char* argv[])
{
signal( SIGTRAP, SIG_IGN );
params p = get_params( argc, argv );
unistd::addrinfo hint = addrinfo{ 0, AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, 0, nullptr, nullptr, nullptr };
std::vector<unistd::addrinfo> addrs = unistd::getaddrinfo( p.hostname, p.port, hint );
const unistd::addrinfo& addr = addrs.at( 0 );
unistd::fd sock = unistd::socket( addr );
subscribe_events( sock );

if ( 0 != p.sndbuf )
    unistd::setsockopt( sock, SOL_SOCKET, SO_SNDBUF, p.sndbuf );

if ( 0 != p.mtu )
    {
    unistd::setsockopt( sock, IPPROTO_IPV6, IPV6_MTU_DISCOVER, IP_PMTUDISC_DONT );
    unistd::setsockopt( sock, IPPROTO_IPV6, IPV6_MTU, p.mtu );
    }

if ( p.nodelay )
    unistd::setsockopt( sock, SOL_SCTP, SCTP_NODELAY, 1 );

if ( 0 != p.max_burst )
    unistd::setsockopt( sock, SOL_SCTP, SCTP_MAX_BURST, p.max_burst );

if ( 0 != p.maxseg )
    unistd::setsockopt( sock, SOL_SCTP, SCTP_MAXSEG, p.maxseg );

int sndbuf = 0;
socklen_t len = sizeof(sndbuf);
getsockopt( sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len );
fprintf( stderr, "sndbuf: set=%d get=%d\n", p.sndbuf, sndbuf );

if ( 0 != p.rcvbuf )
    unistd::setsockopt( sock, SOL_SOCKET, SO_RCVBUF, p.rcvbuf );

sctp_initmsg init_params;
memset( &init_params, 0, sizeof(init_params) );
init_params.sinit_num_ostreams = 1;
init_params.sinit_max_instreams = 1;
init_params.sinit_max_attempts = 3;
init_params.sinit_max_init_timeo = 100;
unistd::setsockopt( sock, SOL_SCTP, SCTP_INITMSG, init_params );

unistd::connect( sock, addr );

sctp_assoc_t assoc_id = 0;
while ( assoc_id == 0 )
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
    if ( hdr.msg_flags & MSG_NOTIFICATION )
        {
        const sctp_notification& notify = *reinterpret_cast<const sctp_notification*>( hdr.msg_iov[0].iov_base );
        switch ( notify.sn_header.sn_type )
            {
            case SCTP_ASSOC_CHANGE:
                {
                const auto& sac = notify.sn_assoc_change;
        std::cout << " assoc_id=" << sac.sac_assoc_id << " error=" << sac.sac_error << " in=" << sac.sac_inbound_streams << " out=" << sac.sac_outbound_streams << std::endl;
                assoc_id = sac.sac_assoc_id;
                break;
                }
            case SCTP_REMOTE_ERROR:
                {
                const auto& sre = notify.sn_remote_error;
                printf( "^^^ remote_error: err=%hu len=%hu\n", ntohs(sre.sre_error), ntohs(sre.sre_length) );
                return EXIT_FAILURE;
                }
            case SCTP_SHUTDOWN_EVENT:
                {
                printf( "^^^ SCTP_SHUTDOWN_EVENT\n" );
                return EXIT_FAILURE;
                }
            }
        }
    }

//int flags = fcntl( sock, F_GETFL, 0 );
//fcntl( sock, F_SETFL, flags | O_NONBLOCK );

const std::vector<char> msg = generate_message( p.msgsize );
struct iovec iov;
iov.iov_base = const_cast<char*>( msg.data() );
iov.iov_len = msg.size();
std::vector<char> cmsg_buff( CMSG_SPACE( sizeof( sctp_sndrcvinfo ) ) );

struct mmsghdr mhdr;
memset( &mhdr, 0, sizeof(mhdr) );
struct msghdr& hdr = mhdr.msg_hdr;
hdr.msg_name = nullptr;
hdr.msg_namelen = 0;
hdr.msg_iov = &iov;
hdr.msg_iovlen = 1;
hdr.msg_flags = 0;
hdr.msg_control = cmsg_buff.data();
hdr.msg_controllen = cmsg_buff.size();

cmsghdr* cmsg = CMSG_FIRSTHDR( &hdr );
cmsg->cmsg_level = IPPROTO_SCTP;
cmsg->cmsg_type = SCTP_SNDRCV;
cmsg->cmsg_len = CMSG_LEN( sizeof( sctp_sndrcvinfo ) );
sctp_sndrcvinfo& cmsg_data = *reinterpret_cast<sctp_sndrcvinfo*>( CMSG_DATA( cmsg ) );
cmsg_data.sinfo_stream = 0;
cmsg_data.sinfo_ssn = 0; //ignored
cmsg_data.sinfo_flags = SCTP_UNORDERED;
cmsg_data.sinfo_ppid = 31337;
cmsg_data.sinfo_context = 123456;
cmsg_data.sinfo_timetolive = 0;
cmsg_data.sinfo_tsn = 0; //ignored
cmsg_data.sinfo_cumtsn = 0; //ignored
cmsg_data.sinfo_assoc_id = assoc_id;

std::vector<mmsghdr> mhdrs( p.batch, mhdr );

while ( p.count )
    {
    const ssize_t count = std::min<uint64_t>( p.batch, p.count );
    ssize_t nsend = 0;
    TEMP_FAILURE_RETRY( nsend = sendmmsg( sock, mhdrs.data(), count, 0 ) );
    if ( 0 == nsend )
        raise( SIGTRAP );
    if ( -1 == nsend )
        raise( SIGTRAP );
    if ( count != nsend )
        raise( SIGTRAP );
    p.count -= count;
    }

//raise( SIGTRAP );

sock.close();
return EXIT_SUCCESS;
}

