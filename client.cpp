#include <signal.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>

#include <iostream>

#include <unistd/fd.hpp>
#include <unistd/netdb.hpp>

int help(FILE* os, int argc, char* argv[])
    {
    fprintf( os, "usage: %s -h [hostname] [port]\n", argv[0] );
    fprintf( os, "  --nodelay   disable Nagle algorithm\n");
    return EXIT_SUCCESS;
    }

const char short_options[] = "h";

struct opt
    {
    enum enum_t
        {
        help = 'h',
        nodelay = 127
        };
    };

static const option long_options[] =
    {
    { "help",       no_argument,        nullptr, opt::help      },
    { "nodelay",    no_argument,        nullptr, opt::nodelay   }
    };

struct params
    {
    bool        nodelay = false;
    std::string hostname = "localhost";
    std::string port = "31337";
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

std::string readln()
    {
    std::string str;
    std::cin >> str;
    return str;
    }

std::vector<char> gen_packet()
{
std::string str = readln();
return std::vector<char>( str.begin(), str.end() );
}

void send_packet(int sock, sctp_assoc_t assoc_id, const std::vector<char>& msg)
{
std::vector<char> cmsg_buff( CMSG_SPACE( sizeof( sctp_sndrcvinfo ) ) );
struct iovec iov;
memset( &iov, 0, sizeof(iov) );
iov.iov_base = const_cast<char*>( msg.data() );
iov.iov_len = msg.size();

struct msghdr hdr;
memset( &hdr, 0, sizeof(hdr) );
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

ssize_t nsend = 0;
TEMP_FAILURE_RETRY( nsend = sendmsg( sock, &hdr, 0 ) );
if ( 0 == nsend )
    raise( SIGTRAP );
if ( -1 == nsend )
    raise( SIGTRAP );
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

int main(int argc, char* argv[])
{
signal( SIGTRAP, SIG_IGN );
params p = get_params( argc, argv );
unistd::addrinfo hint = addrinfo{ 0, AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP, 0, nullptr, nullptr, nullptr };
std::vector<unistd::addrinfo> addrs = unistd::getaddrinfo( p.hostname, p.port, hint );
const unistd::addrinfo& addr = addrs.at( 0 );
unistd::fd sock = unistd::socket( addr );
subscribe_events( sock );

if ( p.nodelay )
    unistd::setsockopt( sock, SOL_SCTP, SCTP_NODELAY, 1 );

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

const std::vector<char> packet_x( { 'p', 'a', 'c', 'k', 'e', 't', '_', 'x' } );
for (;;)
    {
    send_packet( sock, assoc_id, packet_x );
    }

#if 0
//TODO: handle packets
sendmsg
recvmsg
#endif
//raise( SIGTRAP );

sock.close();
return EXIT_SUCCESS;
}

