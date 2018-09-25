/*************************************************************************
  > File Name: SSDPService.h
  > Author: Jiang Xin
  > Mail: Xin.Jiang@harman.com
  > Created Time: 2018-08-17 10:55
 ************************************************************************/

#ifndef __SSDPSERVICE_H__
#define __SSDPSERVICE_H__

#define SSDP_INTERFACE_NAME_LEN    16// IFNAMSIZ
#define SSDP_IP_LEN                16
#define SSDP_PORT                  1900
#define SSDP_FIELD_LEN             128
#define SSDP_LOCATION_LEN          256
#define SSDP_BUFFER_LEN            2048

class CSSDPService
{
public:
    CSSDPService( SSDP_CONTEXT_T *pSSDPContext );
    ~CSSDPService();

// Instance
//public:
    /**
     * @brief Instance method for singlethon pattern
     *
     * @return CSSDPService handler.
     */
    //static CSSDPService* GetInstance()
    //{
    //    static CSSDPService s_SSDPService;
    //    return &s_SSDPService;
    //}

public:
    ORA_VOID  SSDPJoin();
    ORA_VOID  SSDPOffLine();

    ORA_INT32 SSDPNetworkInterfaceUpdate( SSDP_IF_T Interface );
    ORA_INT32 SSDPBroadCastData( const ORA_VOID *pPacket );
    ORA_INT32 SSDPMulticastData( const ORA_VOID *pPacket, struct sockaddr_in address );

private:
    ORA_INT32 SocketCreate();
    ORA_INT32 SocketClose();
    ORA_INT32 SocketRead();
    ORA_INT32 PacketParser( const ORA_CHAR *pData, ORA_INT dataLen, SSDP_PACKET_T *pPacket );

    ORA_INT32 NeighborCheckTimeout();
    ORA_INT32 NeighborListAdd( const SSDP_PACKET_T packet );
    ORA_VOID  NeighborListFree( SSDP_NBR_T *pList );
    ORA_INT32 NeighborRemoveAll();

    ORA_INT32 SendMsearch();
    ORA_INT32 SendResponse( struct sockaddr_in address );

private:
    /** Struct: ssdp packet **/
    typedef struct SSDP_PACKET
    {
        string      Method;   // M-SEARCH, CAST, RESPONSE
        string      St;       // Search Target
        string      Usn;      // Unique Service Name
        string      Location; // Location

        /* Additional SSDP Header Fields */
        string      SmId;
        string      DeviceType;
        ORA_INT64   UpdateTime;
    }SSDP_PACKET_T;

    /* Struct : SSDP_NBR */
    typedef struct SSDP_NBR
    {
        string      Usn;      // Unique Service Name (Device Name or MAC)
        string      Location; // URL or IP(:Port)

        /* Additional SSDP Header Fields */
        string      SmId;
        string      DeviceType;
        ORA_INT64   UpdateTime;
        SSDP_NBR   *Next;
    }SSDP_NBR_T;

    typedef struct SSDP_IF
    {
        string        Name;    // name[16]
        string        Ip;      // ip[16] = "xxx.xxx.xxx.xxx"
        ORA_UINT32    Addr;    // address in network byte order
        ORA_UINT32    Netmask; // mask in network byte order
    }SSDP_IF_T;

    typedef struct SSDP_CONTEXT
    {
        SSDP_IF_T Interface;
        /* Callback Function */
        ORA_INT32 (* NeighborFoundCallback) ( const NW_DEVICE &dev );
        ORA_INT32 (* NeighborLostCallback)  ( const NW_DEVICE &dev );
        ORA_INT32 (* PacketReceivedCallback)( const DEVICE_ID_T &sender, const ORA_VOID *pPacket, ORA_SIZE size );
    }SSDP_CONTEXT_T;

    SSDP_NBR_T      *m_pNeighborList;
    SSDP_CONTEXT_T  *m_pSSDPContext;
    CSSDPUtils      *m_pSSDPUtils;
    ORA_INT32        m_Socket;// SSDP socket
    ORA_UINT16       m_Port;
    ORA_INT64        m_LastTime;
    ORA_UINT64       m_NeighborTimeout;

// Thread Routines
private:
    static ORA_INT_PTR HeartBeatThread( ORA_VOID *param );

// Status
protected:
    ORA_HTHREAD m_hHeartBeatThread;

    mutable ORA_CRITICAL_SECTION m_SocketLock;

};

#endif//#ifndef __SSDPSERVICE_H__
