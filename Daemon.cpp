#include "Base.h"
#include "Daemon.h"

//////////////////////////////////////////////////////////////////////////////
// BEG: CDaemon
CDaemon::CDaemon()
{
    m_pConfig         = ORA_NULL;
    m_hWaitingForExit = ORA_NULL;
}

CDaemon::~CDaemon()
{
    ORA_ASSERT( m_hWaitingForExit == ORA_NULL );
}

ORA_VOID CDaemon::WaitForExit()
{
    ORA_ASSERT( m_hWaitingForExit );
    ORAWaitEvent( m_hWaitingForExit );
}

ORA_BOOL CDaemon::Start()
{
    if( CCommService::Start() )
    {
        m_hWaitingForExit = ORACreateEvent();
        m_pConfig         = CProfile::GetInstance();
        m_pNwSrv          = CNetworkService::GetInstance( this );
        m_pIPCCtrl        = CIPCController::GetInstance();
        ORA_ASSERT( m_pConfig );
        ORA_ASSERT( m_pNwSrv );
        ORA_ASSERT( m_pIPCCtrl );

        m_pRoleManager = new CRoleManager( reinterpret_cast< INwDataDelivery* > ( m_pNwSrv ) );
        if( !m_pRoleManager )
            goto ERR;

        m_pNwSrv->BindNwDataReceiver( m_pRoleManager );

        m_DeviceID = m_pConfig->GetDeviceID();

        RegisterListener( m_pIPCCtrl );
        RegisterListener( m_pNwSrv );
        m_pIPCCtrl->RegisterListener( this );
        m_pIPCCtrl->RegisterListener( m_pNwSrv );
        m_pIPCCtrl->RegisterListener( m_pRoleManager );
        m_pNwSrv->RegisterListener( this );
        m_pNwSrv->RegisterListener( m_pIPCCtrl );
        m_pNwSrv->RegisterListener( m_pNwSrv );

        if( !m_pRoleManager->Start() )
            goto ERR;

        if( !m_pIPCCtrl->Start() )
            goto ERR;

        if( !m_pNwSrv->Start() )
            goto ERR;
    }

ERR:
    if( m_pRoleManager )
    {
        m_pRoleManager->Stop();
        delete m_pRoleManager;
        m_pRoleManager = ORA_NULL;
    }

    UnregisterListener( m_pIPCCtrl );
    UnregisterListener( m_pNwSrv );
    m_pIPCCtrl->UnregisterListener( this );
    m_pIPCCtrl->UnregisterListener( m_pNwSrv );
    m_pNwSrv->UnregisterListener( this );
    m_pNwSrv->UnregisterListener( m_pIPCCtrl );

    m_pIPCCtrl->Stop();
    m_pNwSrv->Stop();
    return ORA_FALSE;
}


ORA_VOID CDaemon::Stop()
{
    ORA_ASSERT( m_pRoleManager );
    UnregisterListener( m_pRoleManager );
    m_pNwSrv->UnregisterListener( m_pRoleManager );
    m_pIPCCtrl->UnregisterListener( m_pRoleManager );
    m_pRoleManager->Stop();
    delete m_pRoleManager;
    m_pRoleManager = ORA_NULL;

    UnregisterListener( m_pIPCCtrl );
    UnregisterListener( m_pNwSrv );

    ORA_ASSERT( m_pNwSrv );
    m_pNwSrv->Stop();
    m_pNwSrv->UnregisterListener( this );
    m_pNwSrv->UnregisterListener( m_pIPCCtrl );

    ORA_ASSERT( m_pIPCCtrl );
    m_pIPCCtrl->Stop();
    m_pIPCCtrl->UnregisterListener( this );
    m_pIPCCtrl->UnregisterListener( m_pNwSrv );

    ORASignalEvent( m_hWaitingForExit );
    ORADestroyEvent( m_hWaitingForExit );
    CCommService::Stop();
}

ORA_VOID CDaemon::OnMsgProcedure( const _MSG_HEAD *pMsg )
{
    ORA_ASSERT( pMsg );
    switch( pMsg->GetMsgID() )
    {
    case MT_NW_PUBLIC_MESH_JOINED:
    {
        if( reinterpret_cast< const FS_MSG_NW_PUBLIC_MESH_JOINED* >( pMsg )->IsJoined() )
        {
            ORA_ASSERT( m_pNwSrv );
            m_pNwSrv->ScanNetwork();

            // TODO: Request IPCCtrl to open BLE
        }
        break;
    }

    case MT_NW_PRIV_MESH_JOINED:
    {
        if( reinterpret_cast< const FS_MSG_NW_PRIV_MESH_JOINED* >( pMsg )->IsJoined() )
        {
            // TODO: Request IPCCtrl to close BLE

            // TODO: Change Role State to NO_ROLE;
            m_pRoleManager->SetState( CRoleManager::RST_NO_ROLE );
        }
        break;
    }

    case MT_NW_SCAN_NETWORK_TIMEOUT:
        // close BLE, and do nothing. fall throught
    case MT_NW_PRIV_MESH_FOUND:
    {
        // TODO: Request IPCCtrl to close BLE, Network service will join private mesh automatically.
        break;
    }

    case MT_IPC_BLE_AP_CONFIGURED:
    {
        // TODO: it perhaps conflicts with MT_NW_PRIV_MESH_FOUND, corner case!!!
        const AP_INFO *apInfo = reinterpret_cast< const FS_MSG_IPC_BLE_AP_CONFIGURED* >( pMsg )->GetApInfo();
        if( m_pNwSrv->ValidateAPConnection( *apInfo ) )
        {
            m_pConfig->AddApInfo( apInfo );
            m_pNwSrv->CreatePrivMesh();
        }
        delete apInfo;
        break;
    }

    default:
        ORA_ASSERT( ORA_FALSE );
    };
}
// END: CDaemon
//////////////////////////////////////////////////////////////////////////////
