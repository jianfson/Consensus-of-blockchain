/*************************************************************************
  > File Name: SSDPService.cpp
  > Author: Jiang Xin
  > Mail: Xin.Jiang@harman.com
  > Created Time: 2018-08-17 10:58
 ************************************************************************/

#include <stdio.h>      // snprintf, vsnprintf
#include <stdlib.h>     // malloc, free
#include <stdarg.h>     // va_start, va_end, va_list
#include <string.h>     // memset, memcpy, strlen, strcpy, strcmp, strncasecmp, strerror
#include <ctype.h>      // isprint, isspace
#include <errno.h>      // errno
#include <unistd.h>     // close
#include <sys/time.h>   // gettimeofday
#include <sys/ioctl.h>  // ioctl, FIONBIO
#include <net/if.h>     // struct ifconf, struct ifreq
#include <fcntl.h>      // fcntl, F_GETFD, F_SETFD, FD_CLOEXEC
#include <sys/socket.h> // struct sockaddr, AF_INET, SOL_SOCKET, socklen_t, setsockopt, socket, bind, sendto, recvfrom
#include <netinet/in.h> // struct sockaddr_in, struct ip_mreq, INADDR_ANY, IPPROTO_IP, also include <sys/socket.h>
#include <arpa/inet.h>  // inet_aton, inet_ntop, inet_addr, also include <netinet/in.h>
#include "SSDPService.h"
#include "SSDPUtils.h"

///////////////////////////////////////////////////////////////////////////////
// BEG: CSSDPService
/**
 * @brief Constructor for SSDP service
 *
 * @param pNetworkService the Network handler
 */
CSSDPService::CSSDPService( SSDP_CONTEXT_T *pSSDPContext )
    : m_pSSDPContext( pSSDPContext )
{
    m_pSSDPUtils = CSSDPUtils::GetInstance();
    ORA_ASSERT( m_pSSDPUtils );

    m_Port = SSDP_PORT;

    ORAInitializeCriticalSection( &m_SocketLock );
    SocketCreate();
}

CSSDPService::~CSSDPService()
{
    ORADeleteCriticalSection( &m_SocketLock );
}

ORA_BOOL CSSDPService::Join()
{
    ORA_ASSERT( this );
    m_LastTime = GetCurrentTime();
    if( m_LastTime < 0 )
    {
        printf("got invalid timestamp %lld\n", m_LastTime);
        return ORA_FALSE;
    }
    //CORASectionLock lock( m_ThreadLock );
    ORA_ASSERT( m_hHeartBeatThread == ORA_NULL );
    m_hHeartBeatThread = ORACreateThread( HeartBeatThread,
                                    reinterpret_cast< ORA_VOID* >( this ),
                                    ORA_TRUE,
                                    ORA_NULL,
                                    ORATP_NORMAL,
                                    DEFAULT_THREAD_STACK_SIZE );
    if( !m_hHeartBeatThread )
        return ORA_FALSE;

    return ORA_TRUE;
}

ORA_VOID CSSDPService::OffLine()
{
    //CORASectionLock lock( m_ThreadLock );

    ORA_MSG msg;
    msg.MessageID = OMID_QUIT;
    ORA_ASSERT( m_hHeartBeatThread );
    ORASendMessage( m_hHeartBeatThread, &msg );
    ORAWaitThreadDead( m_hHeartBeatThread );
    m_hHeartBeatThread = ORA_NULL;
}

ORA_VOID CSSDPService::HeartBeatThread( ORA_VOID *param )
{
    ORA_MSG msg;

    while( ORA_TRUE )
    {
        ORAGetMessage( &msg );
        if( msg.MessageID == OMID_QUIT )
        {
            ORA_INFO_TRACE("HeartBeatThread thread exiting");
            break;
        }
        ORAClearMessage( &msg );

        printf("start to reading new packet...\n");
        CORASectionLock lock( m_SocketLock );
        fd_set fs;
        FD_ZERO( &fs );
        FD_SET( m_Socket, &fs );
        struct timeval tv = {
            .tv_usec = 500 * 1000   // 500 ms
        };

        ORA_INT32 ret = select( m_Socket + 1, &fs, ORA_NULL, ORA_NULL, &tv );
        if( ret < 0 )
        {
            printf("select error, ret = %d\n", ret);
            break;
        }

        if( ret > 0 )
        {
            ReadSocket();
        }

        lock.Unlock();
        // get current time
        ORA_INT64 currentTime = GetCurrentTime();
        if( currentTime < 0 )
        {
            printf("got invalid timestamp %lld\n", currentTime);
            break;
        }

        // doing task per 5 seconds
        if( CurrentTime - m_LastTime >= 5000 )
        {
            SendMsearch();                 // 1. send M-SEARCH
            NeighborCheckTimeout();        // 2. check neighbor timeout

            m_LastTime = CurrentTime;      // update last_time
        }
    }
}

// 01. lssdp_network_interface_update
ORA_INT32 CSSDPService::SSDPNetworkInterfaceUpdate( SSDP_IF_T Interface )
{
    ORA_ASSERT( m_pSSDPContext );

    const ORA_UINT32 sizeOfInterface = sizeof( SSDP_IF_T );

    ORA_INT32 result = 0;

    // compare with original interface
    if ( memcmp(&Interface, &( m_pSSDPContext->Interface ), sizeOfInterface) == 0 )
    {
        // interface is not changed
        return result;
    }

    /* Network Interface is changed */
    CORASectionLock lock( m_SocketLock );
    memset( &( m_pSSDPContext->Interface ), 0, sizeOfInterface );
    memcpy( &( m_pSSDPContext->Interface ), &Interface, sizeOfInterface );
    result = SocketCreate();
    lock.Unlock();

    return result;
}

// 02. lssdp_socket_create
ORA_INT32 CSSDPService::SocketCreate()
{
    ORA_ASSERT( m_pSSDPContext );

    if( m_Port <= 0 )
    {
        printf("SSDP port (%d) has not been setup.\n", m_Port);
        return -1;
    }

    // close original SSDP socket
    SocketClose();

    // create UDP socket
    m_Socket = socket( AF_INET, SOCK_DGRAM, 0 );
    if( m_Socket < 0 )
    {
        printf("create socket failed, errno = %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    ORA_INT32 result = -1;

    // set non-blocking
    ORA_INT32 opt = 1;
    if( ioctl(m_Socket, FIONBIO, &opt) != 0 )
    {
        printf("ioctl FIONBIO failed, errno = %s (%d)\n", strerror(errno), errno);
        goto end;
    }

    // set reuse address
    if( setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0 )
    {
        printf("setsockopt SO_REUSEADDR failed, errno = %s (%d)\n", strerror(errno), errno);
        goto end;
    }

    // set FD_CLOEXEC
    ORA_INT32 sockOpt = fcntl( m_Socket, F_GETFD );
    if(sockOpt == -1)
    {
        printf("fcntl F_GETFD failed, errno = %s (%d)\n", strerror(errno), errno);
    }
    else
    {
        // F_SETFD
        if( fcntl(m_Socket, F_SETFD, sockOpt | FD_CLOEXEC) == -1 )
        {
            printf("fcntl F_SETFD FD_CLOEXEC failed, errno = %s (%d)\n", strerror(errno), errno);
        }
    }

    // bind socket
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons( m_Port ),
        .sin_addr.s_addr = inet_addr( m_pSSDPContext->Interface.Ip.c_str() )
    };
    if( bind(m_Socket, (struct sockaddr *)&addr, sizeof(addr)) != 0 )
    {
        printf("bind failed, errno = %s (%d)\n", strerror(errno), errno);
        goto end;
    }

    // set IP_ADD_MEMBERSHIP
    struct ip_mreq imr = {
        .imr_multiaddr.s_addr = inet_addr( Global.ADDR_MULTICAST ),
        .imr_interface.s_addr = inet_addr( m_pSSDPContext->Interface.Ip.c_str() )
    };
    if( setsockopt(m_Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(struct ip_mreq)) != 0 )
    {
        printf("setsockopt IP_ADD_MEMBERSHIP failed: %s (%d)\n", strerror(errno), errno);
        goto end;
    }

    printf("create SSDP socket %d\n", m_Socket);
    result = 0;
end:
    if( result == -1 )
    {
        SocketClose();
    }
    return result;
}

// 03. lssdp_socket_close
ORA_INT32 CSSDPService::SocketClose()
{
    ORA_ASSERT( m_pSSDPContext );

    // check socket
    if( m_Socket <= 0 )
    {
        printf("SSDP socket is %d, ignore socket_close request.\n", m_Socket);
        goto end;
    }

    // close socket
    if( close(m_Socket) != 0 )
    {
        printf("close socket %d failed, errno = %s (%d)\n", m_Socket, strerror(errno), errno);
        return -1;
    };

    // close socket success
    printf("close SSDP socket %d\n", m_Socket);
end:
    m_Socket = -1;
    NeighborRemoveAll();  // force clean up neighbor_list
    return 0;
}

// 04. lssdp_socket_read
ORA_INT32 CSSDPService::ReadSocket()
{
    ORA_ASSERT( m_pSSDPContext );

    // check socket and port
    if( m_Socket <= 0 )
    {
        printf("SSDP socket (%d) has not been setup.\n", m_Socket);
        return -1;
    }

    if( m_Port == 0 )
    {
        printf("SSDP port (%d) has not been setup.\n", m_Port);
        return -1;
    }

    ORA_CHAR buffer[SSDP_BUFFER_LEN] = {};
    struct sockaddr_in address = {};
    socklen_t address_len = sizeof(struct sockaddr_in);

    ORA_INT recv_len = recvfrom(m_Socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&address, &address_len);
    if( recv_len == -1 )
    {
        printf("recvfrom fd %d failed, errno = %s (%d)\n", m_Socket, strerror(errno), errno);
        return -1;
    }

    // parse SSDP packet to struct
    SSDP_PACKET_T packet = {};
    if( PacketParser(buffer, recv_len, &packet) != 0 )
    {
        return 0;
    }

    // check search target
    if(strcmp(packet.st, lssdp->header.search_target) != 0)
    {
        // search target is not match
        printf("RECV <- %-8s   not match with %-14s %s\n", packet.method, lssdp->header.search_target, packet.location);
        goto end;
    }

    // M-SEARCH: send RESPONSE back
    if(strcmp(packet.method, Global.MSEARCH) == 0) {
        lssdp_send_response(address);
        return 0;
    }

    // RESPONSE, NOTIFY: add to neighbor_list
    NeighborListAdd();

    // RESPONSE: return
    if (strcmp(packet.method, Global.NOTIFY) == 0)
        return 0;

    if (lssdp->debug) {
        lssdp_info("RECV <- %-8s   %-28s  %s\n", packet.method, packet.location, packet.sm_id);
    }
    // invoke packet received callback
    if (lssdp->packet_received_callback != NULL) {
        lssdp->packet_received_callback(lssdp, buffer, recv_len);
    }

    return 0;
}

// 05. lssdp_send_msearch
ORA_INT32 CSSDPService::SSDPSendMsearch()
{
    ORA_ASSERT( m_pSSDPContext );

    if ( m_Port == 0 )
    {
        printf("SSDP port (%d) has not been setup.\n", m_Port);
        return -1;
    }

    // 1. set M-SEARCH packet
    string msearch = m_pSSDPUtils->BuildSSDPSearchString();
    char msearch[LSSDP_BUFFER_LEN] = {};
    snprintf(msearch, sizeof(msearch),
        "%s"
        "HOST:%s:%d\r\n"
        "MAN:\"ssdp:discover\"\r\n"
        "MX:1\r\n"
        "ST:%s\r\n"
        "USER-AGENT:OS/version product/version\r\n"
        "\r\n",
        Global.HEADER_MSEARCH,              // HEADER
        Global.ADDR_MULTICAST, lssdp->port, // HOST
        lssdp->header.search_target         // ST (Search Target)
    );

    // 2. send M-SEARCH to each interface
    size_t i;
    for (i = 0; i < lssdp->interface_num; i++) {
        struct lssdp_interface * interface = &lssdp->interface[i];

        // avoid sending multicast to localhost
        if (interface->addr == inet_addr(Global.ADDR_LOCALHOST)) {
            continue;
        }

        // send M-SEARCH
        int ret = send_multicast_data(msearch, *interface, lssdp->port);
        if (ret == 0 && lssdp->debug) {
            lssdp_info("SEND => %-8s   %s => MULTICAST\n", Global.MSEARCH, interface->ip);
        }
    }

    return 0;
}

// 06. lssdp_send_notify
ORA_INT32 CSSDPService::SSDPSendNotify()
{
    if (lssdp == NULL) {
        lssdp_error("lssdp should not be NULL\n");
        return -1;
    }

    if (lssdp->port == 0) {
        lssdp_error("SSDP port (%d) has not been setup.\n", lssdp->port);
        return -1;
    }

    // check network inerface number
    if (lssdp->interface_num == 0) {
        lssdp_warn("Network Interface is empty, no destination to send %s\n", Global.NOTIFY);
        return -1;
    }

    size_t i;
    for (i = 0; i < lssdp->interface_num; i++) {
        struct lssdp_interface * interface = &lssdp->interface[i];

        // avoid sending multicast to localhost
        if (interface->addr == inet_addr(Global.ADDR_LOCALHOST)) {
            continue;
        }

        // set notify packet
        char notify[LSSDP_BUFFER_LEN] = {};
        char * domain = lssdp->header.location.domain;
        snprintf(notify, sizeof(notify),
            "%s"
            "HOST:%s:%d\r\n"
            "CACHE-CONTROL:max-age=120\r\n"
            "LOCATION:%s%s%s\r\n"
            "SERVER:OS/version product/version\r\n"
            "NT:%s\r\n"
            "NTS:ssdp:alive\r\n"
            "USN:%s\r\n"
            "SM_ID:%s\r\n"
            "DEV_TYPE:%s\r\n"
            "\r\n",
            Global.HEADER_NOTIFY,                       // HEADER
            Global.ADDR_MULTICAST, lssdp->port,         // HOST
            lssdp->header.location.prefix,              // LOCATION
            strlen(domain) > 0 ? domain : interface->ip,
            lssdp->header.location.suffix,
            lssdp->header.search_target,                // NT (Notify Type)
            lssdp->header.unique_service_name,          // USN
            lssdp->header.sm_id,                        // SM_ID    (addtional field)
            lssdp->header.device_type                   // DEV_TYPE (addtional field)
        );

        // send NOTIFY
        int ret = send_multicast_data(notify, *interface, lssdp->port);
        if (ret == 0 && lssdp->debug) {
            lssdp_info("SEND => %-8s   %s => MULTICAST\n", Global.NOTIFY, interface->ip);
        }
    }

    // network inerface is empty
    if (i == 0) lssdp_warn("Network Interface is empty, no destination to send %s\n", Global.NOTIFY);

    return 0;
}

// 07. lssdp_neighbor_check_timeout
ORA_INT32 CSSDPService::NeighborCheckTimeout()
{
    // check neighbor_timeout
    if (m_NeighborTimeout <= 0)
    {
        printf("neighbor_timeout (%ld) is invalid, ignore check_timeout request.\n", m_NeighborTimeout);
        return 0;
    }

    ORA_INT64 currentTime = GetCurrentTime();
    if (currentTime < 0)
    {
        printf("got invalid timestamp %lld\n", currentTime);
        return -1;
    }

    ORA_BOOL isChanged = ORA_FALSE;
    SSDP_NBR_T *prev = ORA_NULL;
    SSDP_NBR_T *nbr  = m_pNeighborList;
    while ( nbr != ORA_NULL )
    {
        ORA_INT64 passTime = currentTime - nbr->UpdateTime;
        if ( passTime < m_NeighborTimeout )
        {
            prev = nbr;
            nbr  = nbr->Next;
            continue;
        }

        isChanged = ORA_TRUE;
        printf("remove timeout SSDP neighbor: %s (%s) (%ldms)\n", nbr->SmId, nbr->Location, passTime);
        // invoke neighbor list lost callback
        if ( m_pSSDPContext->NeighborLostCallback!= ORA_NULL )
        {
            m_pSSDPContext->NeighborLostCallback();
        }

        if (prev == ORA_NULL)
        {
            // it's first neighbor in list
            m_pNeighborList = nbr->next;
            free(nbr);
            nbr = m_pNeighborList;
        }
        else
        {
            prev->Next = nbr->Next;
            free(nbr);
            nbr = prev->Next;
        }
    }

    return 0;
}


/** Internal Function **/

ORA_INT32 CSSDPService::send_multicast_data(const char * data, const struct lssdp_interface interface, unsigned short ssdp_port)
{
    if (data == NULL) {
        lssdp_error("data should not be NULL\n");
        return -1;
    }

    size_t data_len = strlen(data);
    if (data_len == 0) {
        lssdp_error("data length should not be empty\n");
        return -1;
    }

    if (strlen(interface.name) == 0) {
        lssdp_error("interface.name should not be empty\n");
        return -1;
    }

    int result = -1;

    // 1. create UDP socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        lssdp_error("create socket failed, errno = %s (%d)\n", strerror(errno), errno);
        goto end;
    }

    // 2. bind socket
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = interface.addr
    };
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        lssdp_error("bind failed, errno = %s (%d)\n", strerror(errno), errno);
        goto end;
    }

    // 3. disable IP_MULTICAST_LOOP
    char opt = 0;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt)) < 0) {
        lssdp_error("setsockopt IP_MULTICAST_LOOP failed, errno = %s (%d)\n", strerror(errno), errno);
        goto end;
    }

    // 4. set destination address
    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(ssdp_port)
    };
    if (inet_aton(Global.ADDR_MULTICAST, &dest_addr.sin_addr) == 0) {
        lssdp_error("inet_aton failed, errno = %s (%d)\n", strerror(errno), errno);
        goto end;
    }

    // 5. send data
    if (sendto(fd, data, data_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1) {
        lssdp_error("sendto %s (%s) failed, errno = %s (%d)\n", interface.name, interface.ip, strerror(errno), errno);
        goto end;
    }

    result = 0;
end:
    if (fd >= 0 && close(fd) != 0) {
        lssdp_error("close fd %d failed, errno = %s (%d)\n", strerror(errno), errno);
    }
    return result;
}

ORA_INT32 CSSDPService::lssdp_send_response(lssdp_ctx * lssdp, struct sockaddr_in address) {
    // get M-SEARCH IP
    char msearch_ip[LSSDP_IP_LEN] = {};
    if (inet_ntop(AF_INET, &address.sin_addr, msearch_ip, sizeof(msearch_ip)) == NULL) {
        lssdp_error("inet_ntop failed, errno = %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    // 1. find the interface which is in LAN
    struct lssdp_interface * interface = find_interface_in_LAN(lssdp, address.sin_addr.s_addr);
    if (interface == NULL) {
        if (lssdp->debug) {
            lssdp_info("RECV <- %-8s   Interface is not found        %s\n", Global.MSEARCH, msearch_ip);
        }

        if (lssdp->interface_num == 0) {
            lssdp_warn("Network Interface is empty, no destination to send %s\n", Global.RESPONSE);
        }
        return -1;
    }

    // 2. set response packet
    char response[LSSDP_BUFFER_LEN] = {};
    char * domain = lssdp->header.location.domain;
    int response_len = snprintf(response, sizeof(response),
        "%s"
        "CACHE-CONTROL:max-age=120\r\n"
        "DATE:\r\n"
        "EXT:\r\n"
        "LOCATION:%s%s%s\r\n"
        "SERVER:OS/version product/version\r\n"
        "ST:%s\r\n"
        "USN:%s\r\n"
        "SM_ID:%s\r\n"
        "DEV_TYPE:%s\r\n"
        "\r\n",
        Global.HEADER_RESPONSE,                     // HEADER
        lssdp->header.location.prefix,              // LOCATION
        strlen(domain) > 0 ? domain : interface->ip,
        lssdp->header.location.suffix,
        lssdp->header.search_target,                // ST (Search Target)
        lssdp->header.unique_service_name,          // USN
        lssdp->header.sm_id,                        // SM_ID    (addtional field)
        lssdp->header.device_type                   // DEV_TYPE (addtional field)
    );

    // 3. set port to address
    address.sin_port = htons(lssdp->port);

    if (lssdp->debug) {
        lssdp_info("RECV <- %-8s   %s <- %s\n", Global.MSEARCH, interface->ip, msearch_ip);
    }

    // 4. send data
    if (sendto(lssdp->sock, response, response_len, 0, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) == -1) {
        lssdp_error("send RESPONSE to %s failed, errno = %s (%d)\n", msearch_ip, strerror(errno), errno);
        return -1;
    }

    if (lssdp->debug) {
        lssdp_info("SEND => %-8s   %s => %s\n", Global.RESPONSE, interface->ip, msearch_ip);
    }

    return 0;
}

ORA_INT32 CSSDPService::PacketParser(const ORA_CHAR *pData, ORA_INT dataLen, SSDP_PACKET_T *pPacket)
{
    if (pData == ORA_NULL)
    {
        printf("data should not be NULL\n");
        return -1;
    }

    if (dataLen != strlen(pData))
    {
        printf("data_len (%zu) is not match to the data length (%zu)\n", dataLen, strlen(pData));
        return -1;
    }

    if (pPacket == ORA_NULL)
    {
        printf("packet should not be NULL\n");
        return -1;
    }

    // 1. compare SSDP Method Header: M-SEARCH, NOTIFY, RESPONSE
    size_t i;
    if ((i = strlen(Global.HEADER_MSEARCH)) < data_len && memcmp(data, Global.HEADER_MSEARCH, i) == 0) {
        strcpy(packet->method, Global.MSEARCH);
    } else if ((i = strlen(Global.HEADER_NOTIFY)) < data_len && memcmp(data, Global.HEADER_NOTIFY, i) == 0) {
        strcpy(packet->method, Global.NOTIFY);
    } else if ((i = strlen(Global.HEADER_RESPONSE)) < data_len && memcmp(data, Global.HEADER_RESPONSE, i) == 0) {
        strcpy(packet->method, Global.RESPONSE);
    } else {
        printf("received unknown SSDP packet\n");
        printf("%s\n", pData);
        return -1;
    }

    // 2. parse each field line
    size_t start = i;
    for (i = start; i < data_len; i++) {
        if (data[i] == '\n' && i - 1 > start && data[i - 1] == '\r') {
            parse_field_line(data, start, i - 2, packet);
            start = i + 1;
        }
    }

    // 3. set update_time
    long long current_time = get_current_time();
    if (current_time < 0) {
        lssdp_error("got invalid timestamp %lld\n", current_time);
        return -1;
    }
    packet->update_time = current_time;
    return 0;
}

static int parse_field_line(const char * data, size_t start, size_t end, lssdp_packet * packet) {
    // 1. find the colon
    if (data[start] == ':') {
        lssdp_warn("the first character of line should not be colon\n");
        lssdp_debug("%s\n", data);
        return -1;
    }

    int colon = get_colon_index(data, start + 1, end);
    if (colon == -1) {
        lssdp_warn("there is no colon in line\n");
        lssdp_debug("%s\n", data);
        return -1;
    }

    if (colon == end) {
        // value is empty
        return -1;
    }


    // 2. get field, field_len
    size_t i = start;
    size_t j = colon - 1;
    if (trim_spaces(data, &i, &j) == -1) {
        return -1;
    }
    const char * field = &data[i];
    size_t field_len = j - i + 1;


    // 3. get value, value_len
    i = colon + 1;
    j = end;
    if (trim_spaces(data, &i, &j) == -1) {
        return -1;
    };
    const char * value = &data[i];
    size_t value_len = j - i + 1;


    // 4. set each field's value to packet
    if (field_len == strlen("st") && strncasecmp(field, "st", field_len) == 0) {
        memcpy(packet->st, value, value_len < LSSDP_FIELD_LEN ? value_len : LSSDP_FIELD_LEN - 1);
        return 0;
    }

    if (field_len == strlen("nt") && strncasecmp(field, "nt", field_len) == 0) {
        memcpy(packet->st, value, value_len < LSSDP_FIELD_LEN ? value_len : LSSDP_FIELD_LEN - 1);
        return 0;
    }

    if (field_len == strlen("usn") && strncasecmp(field, "usn", field_len) == 0) {
        memcpy(packet->usn, value, value_len < LSSDP_FIELD_LEN ? value_len : LSSDP_FIELD_LEN - 1);
        return 0;
    }

    if (field_len == strlen("location") && strncasecmp(field, "location", field_len) == 0) {
        memcpy(packet->location, value, value_len < LSSDP_LOCATION_LEN ? value_len : LSSDP_LOCATION_LEN - 1);
        return 0;
    }

    if (field_len == strlen("sm_id") && strncasecmp(field, "sm_id", field_len) == 0) {
        memcpy(packet->sm_id, value, value_len < LSSDP_FIELD_LEN ? value_len : LSSDP_FIELD_LEN - 1);
        return 0;
    }

    if (field_len == strlen("dev_type") && strncasecmp(field, "dev_type", field_len) == 0) {
        memcpy(packet->device_type, value, value_len < LSSDP_FIELD_LEN ? value_len : LSSDP_FIELD_LEN - 1);
        return 0;
    }

    // the field is not in the struct packet
    return 0;
}

static int get_colon_index(const char * string, size_t start, size_t end) {
    size_t i;
    for (i = start; i <= end; i++) {
        if (string[i] == ':') {
            return i;
        }
    }
    return -1;
}

static int trim_spaces(const char * string, size_t * start, size_t * end) {
    int i = *start;
    int j = *end;

    while (i <= *end   && (!isprint(string[i]) || isspace(string[i]))) i++;
    while (j >= *start && (!isprint(string[j]) || isspace(string[j]))) j--;

    if (i > j) {
        return -1;
    }

    *start = i;
    *end   = j;
    return 0;
}

ORA_UINT64 CSSDPService::GetCurrentTime() {
    struct timeval time = {};
    if (gettimeofday(&time, NULL) == -1) {
        lssdp_error("gettimeofday failed, errno = %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    return (ORA_UINT64) time.tv_sec * 1000 + (ORA_UINT64) time.tv_usec / 1000;
}

ORA_INT32 NeighborListAdd(const SSDP_PACKET_T packet)
{
    SSDP_NBR_T *pLastNbr = m_pNeighborList;

    ORA_BOOL isChanged = ORA_FALSE;
    SSDP_NBR_T *pNbr;
    for ( pNbr = m_pNeighborList; pNbr != ORA_NULL; pLastNbr = pNbr, pNbr = pNbr->Next )
    {
        if ( strcmp(pNbr->Location, packet.Location) != 0 )
        {
            // location is not match
            continue;
        }

        /* location is not found in SSDP list: update neighbor */

        // usn
        if (strcmp(nbr->usn, packet.usn) != 0) {
            lssdp_debug("neighbor usn is changed. (%s -> %s)\n", nbr->usn, packet.usn);
            memcpy(nbr->usn, packet.usn, LSSDP_FIELD_LEN);
            is_changed = true;
        }

        // sm_id
        if (strcmp(nbr->sm_id, packet.sm_id) != 0) {
            lssdp_debug("neighbor sm_id is changed. (%s -> %s)\n", nbr->sm_id, packet.sm_id);
            memcpy(nbr->sm_id, packet.sm_id, LSSDP_FIELD_LEN);
            is_changed = true;
        }

        // device type
        if (strcmp(nbr->device_type, packet.device_type) != 0) {
            lssdp_debug("neighbor device_type is changed. (%s -> %s)\n", nbr->device_type, packet.device_type);
            memcpy(nbr->device_type, packet.device_type, LSSDP_FIELD_LEN);
            is_changed = true;
        }

        // update_time
        nbr->update_time = packet.update_time;
        goto end;
    }


    /* location is not found in SSDP list: add to list */

    // 1. memory allocate lssdp_nbr
    nbr = (lssdp_nbr *) malloc(sizeof(lssdp_nbr));
    if (nbr == NULL) {
        lssdp_error("malloc failed, errno = %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    // 2. setup neighbor
    memcpy(nbr->usn,         packet.usn,         LSSDP_FIELD_LEN);
    memcpy(nbr->sm_id,       packet.sm_id,       LSSDP_FIELD_LEN);
    memcpy(nbr->device_type, packet.device_type, LSSDP_FIELD_LEN);
    memcpy(nbr->location,    packet.location,    LSSDP_LOCATION_LEN);
    nbr->update_time = packet.update_time;
    nbr->next = NULL;

    // 3. add neighbor to the end of list
    if (last_nbr == NULL) {
        // it's the first neighbor
        lssdp->neighbor_list = nbr;
    } else {
        last_nbr->next = nbr;
    }

    is_changed = true;
end:
    // invoke neighbor list changed callback
    if (lssdp->neighbor_list_changed_callback != NULL && is_changed == true) {
        lssdp->neighbor_list_changed_callback(lssdp);
    }

    return 0;
}

ORA_INT32 CSSDPService::NeighborRemoveAll()
{
    if (m_pNeighborList == ORA_NULL)
    {
        return 0;
    }

    // free neighbor_list
    NeighborListFree(m_pNeighborList);
    m_pNeighborList = ORA_NULL;

    printf("neighbor list has been force clean up.\n");

    return 0;
}

ORA_VOID CSSDPService::NeighborListFree(SSDP_NBR_T *List)
{
    if ( List != ORA_NULL )
    {
        // invoke neighbor list lost callback
        if (m_pSSDPContext->NeighborLostCallback != ORA_NULL)
        {
            m_pSSDPContext->NeighborLostCallback(lssdp);
        }
        NeighborListFree( List->Next );
        free(List);
    }
}

static struct lssdp_interface * find_interface_in_LAN(lssdp_ctx * lssdp, uint32_t address) {
    struct lssdp_interface * ifc;
    size_t i;
    for (i = 0; i < lssdp->interface_num; i++) {
        ifc = &lssdp->interface[i];

        // mask address to check whether the interface is under the same Local Network Area or not
        if ((ifc->addr & ifc->netmask) == (address & ifc->netmask)) {
            return ifc;
        }
    }
    return NULL;
}
