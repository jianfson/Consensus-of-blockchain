#ifndef __FS_NETWORK_SERVICE_H__
#define __FS_NETWORK_SERVICE_H__

#include "Common.h"
#include "Profile.h"
#include "CommService.h"

class CDaemon;
class CNetworkService : public CCommService, public INwDeviceDiscovery, public INwDataReceiver
{
// Constructor & Destructor
private:
    CNetworkService( CDaemon *pDaemon );
    ~CNetworkService();

// Instance
public:
    static CNetworkService* GetInstance( CDaemon *pDaemon )
    {
        ORALOG_ASSERT(pDaemon);
        static CNetworkService s_NwSrv( pDaemon );
        return &s_NwSrv;
    }

// Operations
public:
    /**
     * @brief Make device is visible on the public mesh network,
     * and scan current environment if exists private mesh network
     */
    ORA_VOID ScanNetwork();

    /**
     * @brief Create the private mesh, and issued this private mesh info.
     */
    ORA_VOID CreatePrivMesh();

    /**
     * @brief Validate specified AP if it can be connected successfully.
     * @note it is a sync call for IPC request, there is a sync mutex lock here.
     * @param apInfo AP information for connecting.
     *
     * @return ORA_TRUE if connected successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL ValidateAPConnection( const AP_INFO &apInfo );

    /**
     * @brief Bind the network data receiver for receiving data
     *
     * @param recv  INwDataReceiver interface.
     */
    ORA_VOID BindNwDataReceiver( INwDataReceiver *recv );

    /**
     * @brief Connect the external network via AP connection.
     * @note the function should be used under Master Status.
     */
    ORA_VOID ConnectExternalNetwork();

    /**
     * @brief get AP connection status for current device
     *
     * @return AP connection status:
     */
    NwConnStat GetAPConnStatus();

// Overrides
public:
    /**
     * @brief start the fast setup Network Service
     *
     * @return ORA_TRUE if start successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL Start();

    /**
     * @brief stop the fast setup Network Service
     */
    ORA_VOID Stop();

    /**
     * @brief Broadcast Data packet to current network via UDP connection
     *
     * @param pPacket Data packet
     *
     * @return broadcasted the data size
     */
    ORA_VOID BroadcastDataPacket( const ORA_VOID *pPacket );

    /**
     * @brief Multi-cast Data packet to current network via UDP connection
     *
     * @param pPacket Data packet
     *
     * @return broadcasted the data size
     */
    ORA_VOID MulticastDataPacket( const CDevIDList &targetIDs, const ORA_VOID *pPacket );

    /**
     * @brief Uni-cast Data packet to current network via UDP connection
     *
     * @param pPacket Data packet
     *
     * @return broadcasted the data size
     */
    ORA_VOID UnicastDataPacket( DEVICE_ID_T targetID, const ORA_VOID *pPacket );

    /**
     * @brief Send data packet to target network devices via TCP connection
     *
     * @param targetIDs  Target network device IDs
     * @param pPacket    Data packet
     *
     * @return sent the data packet size
     */
    ORA_VOID SendDataPacket( const CDevIDList &targetIDs, const ORA_VOID *pPacket );

// Overrides
private:
    ORA_INT32 NetworkInterfaceChanged(){ return 0; }
    ORA_INT32 NeighborDeviceFound( const NW_DEVICE &dev ){ return 0; }
    ORA_INT32 NeighborDeviceLost( const NW_DEVICE &dev ){ return 0; }

    /**
     * @brief receive data packet from sender device
     *
     * @param sender    from which sent the data packet
     * @param pPacket   the data packet
     * @param size      the data packet's size
     */
    ORA_VOID RecvDataPacket( const DEVICE_ID_T &sender, const ORA_VOID *pPacket, ORA_SIZE size );

    ORA_VOID OnMsgProcedure( const _MSG_HEAD *pMsg );

// Assistants
private:
    ORA_VOID PrivateMeshNetworkFound( const MESH_INFO &mInfo );
    ORA_VOID JoinMeshNetwork( const MESH_INFO &mInfo );
    ORA_VOID LeaveMeshNetwork();

// Thread Routines
private:
//    static ORA_VOID* ####Thread( ORA_VOID *pContext );

// Properties
private:
    struct NW_NEIGHBOR
    {
        NW_DEVICE Info;
        ORA_BOOL  IsNeighbor;
    };

    typedef vector< NW_NEIGHBOR > CNwNeighborList;

    CDaemon         *m_pDaemon;
    CProfile        *m_pConfig;

    MESH_INFO        m_PublicMeshInfo;
    NwConnStat       m_PublicNwStat;   ///< Public Mesh Network Connection Status
    ORA_INT32        m_UserID;

    MESH_INFO        m_PrivMeshInfo;
    NwConnStat       m_PrivNwStat;     ///< Private Mesh Network Connection Status
    ORA_INT32        m_GroupID;
    ORA_BOOL         m_bApValid;

    CNwNeighborList  m_NeighborList;

    ORA_HTHREAD      m_hMsgProcedureThread;
    ORA_HTIMER       m_hTimer;
    INwDataReceiver *m_pDataRecv;
    NwConnStat       m_APConnStat;
    mutable ORA_CRITICAL_SECTION m_NeighborListLock;  ///< Lock m_NeighborList
    mutable ORA_HEVENT m_hMsgSyncEvent;

    CSSDPService    *m_pSSDPService;
};
#endif /* __FS_NETWORK_SERVICE_H__ */
