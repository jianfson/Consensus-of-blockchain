#include "Base.h"
#include "Daemon.h"
#include "IPCCtrl.h"

ORA_CHAR IPC_FAST_SETUP[] = "ora.ipc.fastsetup"; //!!TBR, it should be defined in ora_ipc_module_fastsetup.h

/**
 * @brief Constructor for CIPCController
 */
CIPCController::CIPCController()
{
}

/**
 * @brief Destructor for CIPCController
 */
CIPCController::~CIPCController()
{
}

/**
 * @brief Initialize the IPC Controller, register to IPC daemon.
 *
 * @return ORA_TRUE if initialized successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CIPCController::Start()
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
        return ORA_TRUE;
    }

    return ORA_FALSE;

    if( ora_ipc_init(IPC_FAST_SETUP, EventProcess, RequestMsgProcess, ReplyMsgProcess) != ORA_IPC_RET_OK )
        return ORA_FALSE;

    // TODO: initialize internal logic
    ORA_INFO_TRACE("Ora IPC initialed");

    if( ORA_IPC_RET_OK == ora_ipc_is_module_ready( (char*)AscServiceName.c_str() ) )
    {
        ORA_INFO_TRACE("Ora asc is ready");
        if( ORA_IPC_RET_OK != ora_ipc_subscribe_event( (char*)AscChargerEvent.c_str() ) )
        {
            ORA_INFO_TRACE("Ora asc %s is fail",(char*)AscChargerEvent.c_str());
            return;
        }

        if( ORA_IPC_RET_OK != ora_ipc_subscribe_event( (char*)AscBatteryEvent.c_str() ))
        {
            ORA_INFO_TRACE("Ora asc %s is fail",(char*)AscBatteryEvent.c_str());
            return;
        }

        ORA_INFO_TRACE("Ora IPC service is ready");
        m_isReady = ORA_TRUE;
    }
    return ORA_TRUE;
}

/**
 * @brief Uninitialize the IPC Controller, unregister from IPC daemon.
 */
ORA_VOID CIPCController::Stop()
{
    ora_ipc_close();
}


/**
 * @brief callback function prototype for incoming event handling
 * @param sender sender module name of the event
 * @param event  event name
 * @param value  event data
**/
ORA_VOID CIPCController::EventProcess(const ORA_CHAR *sender, const ORA_CHAR *event, const ORA_IPC_MSG *value)
{
    CIPCController *pIPCController = CIPCController::GetInstance();
    ORA_ASSERT( pIPCController );

    //! TODO: wrap to _MSG_HEAD message, and NotifyEvent directly.
    string servierName(sender);
    string eventName(event);
    if( servierName == AscServiceName )
    {
        if( AscChargerEvent == eventName )
        {
        }
        else if( AscBatteryEvent == eventName )
        {
            const string batteryLevelStr("battery_level");
            ORA_INT32 batteryLevel = ora_ipc_msg_get_int(value,(char*)batteryLevelStr.c_str());
            UPG_MSG_QUERY_BATTERY_RESP batteryRESP(batteryLevel);

            IPCService::GetInstance(NULL)->NotifyMsg(&batteryRESP,sizeof(batteryRESP));

        }
    }
}

/**
 * @brief callback function prototype for incoming request handling
 * @param sender sender module name of the request
 * @param msgid  request name
 * @param value  request data
 */
ORA_VOID CIPCController::RequestMsgProcess(const ORA_CHAR *sender, const ORA_CHAR *msgid, const ORA_IPC_MSG *value)
{
    CIPCController *pIPCController = CIPCController::GetInstance();
    ORA_ASSERT( pIPCController );

    //! TODO: wrap to _MSG_HEAD message, and NotifyEvent directly.
}

/**
 * @brief callback function prototype for incoming reply handling
 * @param sender sender module name of the reply
 * @param msgid  reply name
 * @param value  reply data
 **/
ORA_VOID CIPCController::ReplyMsgProcess(const ORA_CHAR *sender, const ORA_CHAR *msgid, const ORA_IPC_MSG *value)
{
    CIPCController *pIPCController = CIPCController::GetInstance();
    ORA_ASSERT( pIPCController );

    //! TODO: wrap to _MSG_HEAD message, and NotifyEvent directly.
    string servierName(sender);
    string msgID(msgid);
    if( servierName == AscServiceName )
    {
        if( AscBatteryRESP == msgID )
        {
            string batteryLevelStr("battery_level");
            ORA_INT32 batteryLevel = ora_ipc_msg_get_int(value,(char*)batteryLevelStr.c_str());
            UPG_MSG_QUERY_BATTERY_RESP batteryRESP(batteryLevel);

            IPCService::GetInstance(NULL)->NotifyMsg(&batteryRESP,sizeof(batteryRESP));
        }
    }
}

/**
 * @brief Process internal message from other components
 *
 * @param pMsg _MSG_HEAD item
 */
ORA_VOID CIPCController::OnMsgProcedure( const _MSG_HEAD *pMsg )
{
    ORA_ASSERT( pMsg );
    switch( pMsg->GetMsgID() )
    {
    // TODO: add relative message response here!!
    default:
        ORA_ASSERT( ORA_FALSE );
    }
}
