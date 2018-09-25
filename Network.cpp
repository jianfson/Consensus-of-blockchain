#include "Base.h"
#include "Network.h"
#include "Daemon.h"

#define PUBLIC_MESH_ESSID_PREFIX  "ora_mesh_"
#define PRIVATE_MESH_ESSID_PREFIX "unique_ssid_ora_mesh_"
#define DEFAULT_MESH_CHANNEL      6

///////////////////////////////////////////////////////////////////////////////
// BEG: CNetworkService
/**
 * @brief Constructor for Network service
 *
 * @param pDaemon the daemon handler
 */
CNetworkService::CNetworkService(CDaemon *pDaemon)
    : m_pDaemon( pDaemon )
{
    m_pConfig = ORA_NULL;
    m_UserID  = 0;
    m_PrivNwStat = NCS_NONE;
    m_PublicNwStat = NCS_NONE;
    m_hMsgSyncEvent = ORA_NULL;
    m_pDataRecv = ORA_NULL;
}

CNetworkService::~CNetworkService()
{
    // do nothing.
}

/**
 * @brief start the fast setup Network Service
 * Network start procedure:
 * 1. check profile, and detect which mesh network used;
 *     a) if exists private mesh profile, use private directly;
 *     b) otherwise, load public mesh profile, and use public one.
 * @return ORA_TRUE if start successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CNetworkService::Start()
{
    if( CCommService::Start() )
    {
        ORA_ASSERT( m_pConfig == ORA_NULL );
        ORA_ASSERT( m_hMsgSyncEvent == ORA_NULL );

        m_hMsgSyncEvent = ORACreateEvent();
        ORA_ASSERT( m_hMsgSyncEvent );

        m_pConfig = CProfile::GetInstance();
        ORA_ASSERT( m_pConfig );

        m_UserID       = m_pConfig->GetUserID();
        m_GroupID      = m_pConfig->GetGroupID();

        SSDP_CONTEXT_T ssdpContext =
        {
            .Interface =
            {
                .search_target       = "ST_P2P",
                .unique_service_name = "f835dd000001",
                .sm_id               = "700000123",
                .device_type         = "DEV_TYPE",
                .location.suffix     = ":5678"
            },

            // callback
            .NeighborFoundCallback  = NeighborDeviceFound,
            .NeighborLostCallback   = NeighborDeviceLost,
            .PacketReceivedCallback = RecvDataPacket
        };

        m_pSSDPService = new CSSDPService( &ssdpContext );
        ORA_ASSERT( m_pSSDPService );

        m_PublicMeshInfo = m_pConfig->GetPublicMeshInfo();
        if( !m_PublicMeshInfo.IsValid() )
        {
            ORA_CHAR essid[64];
            memset( essid, 0, 64 );
            sprintf( essid, "%s%d", PUBLIC_MESH_ESSID_PREFIX, m_UserID );
            m_PublicMeshInfo.ESSID   = essid;
            m_PublicMeshInfo.SubMask = "255.0.0.0";

            // TODO: assign a IP Address according to MAC Address
            m_PublicMeshInfo.IpAddr  = "10.1.2.3";
            m_PublicMeshInfo.Channel = DEFAULT_MESH_CHANNEL;

            m_pConfig->SetPublicMeshInfo( &m_PublicMeshInfo );
        }

        m_PrivMeshInfo = m_pConfig->GetPrivMeshInfo();
        if( m_PrivMeshInfo.IsValid() )
        {
            m_PrivNwStat = NCS_CONNECTING;
            JoinMeshNetwork( m_PrivMeshInfo );
        }
        else
        {
            m_PublicNwStat = NCS_CONNECTING;
            JoinMeshNetwork( m_PublicMeshInfo );
        }

        m_pSSDPService->join();

        return ORA_TRUE;
    }

    return ORA_FALSE;
}

/**
 * @brief stop the fast setup Network Service
 */
ORA_VOID CNetworkService::Stop()
{
    ORA_ASSERT( m_hMsgSyncEvent );
    ORADestroyEvent( m_hMsgSyncEvent );
    m_hMsgSyncEvent = ORA_NULL;

    CCommService::Stop();
}

ORA_BOOL CNetworkService::ValidateAPConnection( const AP_INFO &apInfo )
{
    ORA_BOOL bValid = ORA_FALSE;

    NotifyEvent( FS_MSG_IPC_AP_CONNECT( apInfo ) );
    ORAWaitEvent( m_hMsgSyncEvent );

    bValid = m_bApValid;
    m_bApValid = ORA_FALSE;

    if( bValid )
        NotifyEvent( FS_MSG_IPC_AP_DISCONNECT() );

    return bValid;
}

/**
 * @brief Bind the network data receiver for receiving data
 *
 * @param recv  INwDataReceiver interface.
 */
ORA_VOID CNetworkService::BindNwDataReceiver( INwDataReceiver *recv )
{
    m_pDataRecv = recv;
}


/**
 * @brief Connect the external network via AP connection.
 * @note the function should be used under Master Status.
 */
ORA_VOID CNetworkService::ConnectExternalNetwork()
{
    m_APConnStat = NCS_CONNECTING;
    NotifyEvent( FS_MSG_IPC_AP_CONNECT( AP_INFO() ) );
    ORAWaitEvent( m_hMsgSyncEvent );
}

/**
 * @brief get AP connection status for current device
 *
 * @return AP connection status:
 */
NwConnStat CNetworkService::GetAPConnStatus()
{
    //! TBD: query ap connection via IPC message, and use sync call.
    return m_APConnStat;
}

/**
 * @brief Make device is visible on the public mesh network,
 * and scan current environment if exists private mesh network
 */
ORA_VOID CNetworkService::ScanNetwork()
{
    NotifyEvent( FS_MSG_IPC_SCAN_PRIV_MESH() );
    // TODO: start SSDP scanning ... (Timeout impl within SSDP)
    m_pSSDPService->SendMSearch();
    // INwDeviceDiscory interface need be used within SSDP for different status.
}

/**
 * @brief Create the private mesh, and issued this private mesh info.
 */
ORA_VOID CNetworkService::CreatePrivMesh()
{
    // TODO: conflict handling ...
    if( m_PrivNwStat == NCS_CONNECTED )
        return;

    ORA_CHAR essid[64];
    memset( essid, 0, 64 );
    sprintf( essid, "%s%d_%d", PRIVATE_MESH_ESSID_PREFIX, m_UserID, m_GroupID );
    m_PrivMeshInfo.ESSID   = essid;
    m_PrivMeshInfo.SubMask = "255.0.0.0";

    // TODO: assign a IP Address according to MAC Address
    m_PrivMeshInfo.IpAddr  = "10.1.2.3";
    m_PrivMeshInfo.Channel = DEFAULT_MESH_CHANNEL;
    m_pConfig->SetPrivMeshInfo( &m_PrivMeshInfo );

    NotifyEvent( FS_MSG_NW_PRIV_MESH_FOUND() );
    PrivateMeshNetworkFound( m_PrivMeshInfo );
}

/**
 * @brief receive data packet from sender device
 *
 * @param sender    from which sent the data packet
 * @param pPacket   the data packet
 * @param size      the data packet's size
 */
ORA_VOID CNetworkService::RecvDataPacket( const DEVICE_ID_T &sender, const ORA_VOID *pPacket, ORA_SIZE size )
{
    if( m_pDataRecv )
        m_pDataRecv->RecvDataPacket( sender, pPacket, size );
}

/**
 * @brief message procedure routine for internal messages.
 *
 * @param pMsg internal messages
 */
ORA_VOID CNetworkService::OnMsgProcedure( const _MSG_HEAD *pMsg )
{
    ORA_ASSERT( pMsg );
    switch( pMsg->GetMsgID() )
    {
    case MT_IPC_SET_MESH_INFO_RESP:
        ORASignalEvent( m_hMsgSyncEvent );
        break;

    case MT_IPC_START_MESH_RESP:
    {
        ORA_BOOL isStarted = reinterpret_cast< const FS_MSG_IPC_START_MESH_RESP* >( pMsg )->IsStarted();
        ORA_INT  errCode   = reinterpret_cast< const FS_MSG_IPC_START_MESH_RESP* >( pMsg )->GetErrCode();

        if( m_PrivNwStat == NCS_CONNECTING )
        {
            NotifyEvent( FS_MSG_NW_PRIV_MESH_JOINED( isStarted, errCode ) );
            m_PrivNwStat = isStarted ? NCS_CONNECTED : NCS_DISCONNECTED;
            ORASignalEvent( m_hMsgSyncEvent );

            if( !isStarted )
            {
                //! TBD: if need re-join the private for several times, and then switch to public mesh after failed many times.
                m_PublicNwStat = NCS_CONNECTING;
                JoinMeshNetwork( m_PublicMeshInfo );    // switch to public mesh automatically.
            }
        }
        else if( m_PublicNwStat == NCS_CONNECTING )
        {
            NotifyEvent( FS_MSG_NW_PUBLIC_MESH_JOINED( isStarted, errCode ) );
            m_PublicNwStat = isStarted ? NCS_CONNECTED : NCS_DISCONNECTED;
            ORASignalEvent( m_hMsgSyncEvent );
        }
        else
            ORA_ASSERT( ORA_FALSE );

        break;
    }

    case MT_IPC_STOP_MESH_RESP:
    {
        if( m_PrivNwStat == NCS_CONNECTED )
        {
//            NotifyEvent( FS_MSG_NW_PRVI_MESH_LEFT() );
            m_PrivNwStat = NCS_DISCONNECTED;
        }
        else if( m_PublicNwStat == NCS_CONNECTED )
        {
//            NotifyEvent( FS_MSG_NW_PRVI_MESH_LEFT() );
            m_PublicNwStat = NCS_DISCONNECTED;
        }
        else
            ORA_ASSERT( ORA_FALSE );

        ORASignalEvent( m_hMsgSyncEvent );
        break;
    }

    case MT_IPC_SCAN_PRIV_MESH_RESP:
    {
        if( reinterpret_cast< const FS_MSG_IPC_SCAN_PRIV_MESH_RESP* >( pMsg )->IsTimeout() )
            NotifyEvent( FS_MSG_NW_SCAN_NETWORK_TIMEOUT() ); // If public scan timeout (3min), do nothing.
        else
        {
            NotifyEvent( FS_MSG_NW_PRIV_MESH_FOUND() );
            PrivateMeshNetworkFound(
                    *reinterpret_cast< const FS_MSG_IPC_SCAN_PRIV_MESH_RESP* >( pMsg )->GetMeshInfo() );
        }
        break;
    }

    case MT_IPC_BLE_AP_CONFIGURED:
    {
        // Do nothing.
        // handled by CDaemon
        break;
    }

    case MT_IPC_AP_CONNECT_RESP:
        ORASignalEvent( m_hMsgSyncEvent );
        m_APConnStat = reinterpret_cast< const FS_MSG_IPC_AP_CONNECT_RESP* >( pMsg )->IsConnected() ? NCS_CONNECTED : NCS_DISCONNECTED;
        break;

    case MT_IPC_AP_DISCONNECT_RESP:
        m_APConnStat = NCS_DISCONNECTED;
        break;
    }
}


/**
 * @brief found available private mesh network during the ssdp scanning.
 *
 * @param mInfo
 */
ORA_VOID CNetworkService::PrivateMeshNetworkFound( const MESH_INFO &mInfo )
{
    ORA_ASSERT( m_PrivNwStat == NCS_NONE );
    m_PrivMeshInfo = mInfo;

    ORA_ASSERT( m_PublicNwStat == NCS_CONNECTED );
    LeaveMeshNetwork();

    m_PrivNwStat = NCS_CONNECTING;
    JoinMeshNetwork( m_PrivMeshInfo );
}

ORA_VOID CNetworkService::JoinMeshNetwork( const MESH_INFO &mInfo )
{
    ORA_ASSERT( mInfo.IsValid() );

    NotifyEvent( FS_MSG_IPC_SET_MESH_INFO( mInfo ) );
    ORAWaitEvent( m_hMsgSyncEvent );

    ORAResetEvent( m_hMsgSyncEvent );
    NotifyEvent( FS_MSG_IPC_START_MESH() );
    ORAWaitEvent( m_hMsgSyncEvent );
}

ORA_VOID CNetworkService::LeaveMeshNetwork()
{
    ORA_ASSERT( m_PublicNwStat == NCS_CONNECTED || m_PrivNwStat == NCS_CONNECTED );
    NotifyEvent( FS_MSG_IPC_STOP_MESH() );
    ORAWaitEvent( m_hMsgSyncEvent );
    m_PublicNwStat = NCS_DISCONNECTED;
    m_PrivNwStat   = NCS_DISCONNECTED;
}

/**
 * @brief Broadcast Data packet to current network via UDP connection
 *
 * @param pPacket Data packet
 *
 * @return broadcasted the data size
 */
ORA_VOID CNetworkService::BroadcastDataPacket( const ORA_VOID *pPacket );

/**
 * @brief Multi-cast Data packet to current network via UDP connection
 *
 * @param pPacket Data packet
 *
 * @return broadcasted the data size
 */
ORA_VOID CNetworkService::MulticastDataPacket( const CDevIDList &targetIDs, const ORA_VOID *pPacket );

/**
 * @brief Uni-cast Data packet to current network via UDP connection
 *
 * @param pPacket Data packet
 *
 * @return broadcasted the data size
 */
ORA_VOID CNetworkService::UnicastDataPacket( DEVICE_ID_T targetID, const ORA_VOID *pPacket );

ORA_INT32 CNetworkService::NetworkInterfaceChanged()
{
    return 0;
}
ORA_INT32 CNetworkService::NeighborDeviceFound( const NW_DEVICE &dev ){ return 0; }
ORA_INT32 CNetworkService::NeighborDeviceLost( const NW_DEVICE &dev ){ return 0; }
// END: CNetworkService
///////////////////////////////////////////////////////////////////////////////
