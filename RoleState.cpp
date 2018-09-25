#include "Base.h"
#include "RoleState.h"

//////////////////////////////////////////////////////////////////////////////
// BEG: CRoleManager
/**
 * @brief constructor for CRoleManager
 *
 * @param pDelivery the network data delivery interface.
 */
CRoleManager::CRoleManager( INwDataDelivery* pDelivery )
{
    ORA_ASSERT( pDelivery );
    m_pDelivery  = pDelivery;
    m_pCurrState = ORA_NULL;
}

CRoleManager::~CRoleManager()
{
    // Do nothing;
}

/**
 * @brief start the role manager.
 *
 * @return ORA_TRUE if successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CRoleManager::Start()
{
    if( CCommService::Start() )
    {
        CRoleState *pNoRole  = ORA_NULL;
        CRoleState *pPreRole = ORA_NULL;
        CRoleState *pDefiner = ORA_NULL;
        CRoleState *pSlave   = ORA_NULL;
        CRoleState *pMaster  = ORA_NULL;

        pNoRole = new CNoRoleState( this );
        if( pNoRole )
            m_RoleStateMap.insert( CRoleStateMap::value_type( RST_NO_ROLE, pNoRole ) );
        else
            goto ERR;

        pPreRole = new CPreRoleState( this );
        if( pPreRole )
            m_RoleStateMap.insert( CRoleStateMap::value_type( RST_NO_ROLE, pPreRole ) );
        else
            goto ERR;

        pDefiner = new CDefinerState( this );
        if( pDefiner )
            m_RoleStateMap.insert( CRoleStateMap::value_type( RST_NO_ROLE, pDefiner ) );
        else
            goto ERR;

        pSlave = new CSlaveState( this );
        if( pSlave )
            m_RoleStateMap.insert( CRoleStateMap::value_type( RST_NO_ROLE, pSlave ) );
        else
            goto ERR;

        pMaster = new CMasterState( this );
        if( pMaster )
            m_RoleStateMap.insert( CRoleStateMap::value_type( RST_NO_ROLE, pMaster ) );
        else
            goto ERR;

        return ORA_TRUE;
    }

ERR:
    while( m_RoleStateMap.size() )
    {
        CRoleStateMap::iterator it = m_RoleStateMap.begin();
        ORA_ASSERT( it->second );
        delete it->second;
        m_RoleStateMap.erase( it );
    }
    return ORA_FALSE;
}

/**
 * @brief stop the role manager.
 */
ORA_VOID CRoleManager::Stop()
{
    while( m_RoleStateMap.size() )
    {
        CRoleStateMap::iterator it = m_RoleStateMap.begin();
        ORA_ASSERT( it->second );
        delete it->second;
        m_RoleStateMap.erase( it );
    }
    CCommService::Stop();
}

/**
 * @brief set current state to specified state
 *
 * @param state   RoleStateType value.
 * @param pParam  Allow the caller passes by a parameter to new state.
 * @param bForced force to deactive previous status
 * @note: if forced flag used, it perhaps break out the previous state's handling procedure.
 */
ORA_VOID CRoleManager::SetState( RoleStateType state, ORA_VOID *pParam /* = ORA_NULL */, ORA_BOOL bForced /* = ORA_FALSE */ )
{
    ORA_ASSERT( state < RST_STATE_TYPE_COUNT );
    if( m_pCurrState )
        m_pCurrState->Deactivate( bForced );

    CRoleState *pNewStat = m_RoleStateMap.find( state )->second;
    ORA_ASSERT( pNewStat );
    pNewStat->Activate( pParam );
    m_pCurrState = pNewStat;
}

/**
 * @brief send the event the all devices via broadcast approach.
 *
 * @param pEvent the event data
 */
ORA_VOID CRoleManager::SendEvent( const ROLE_EVENT *pEvent )
{
    ORA_ASSERT( pEvent && pEvent->IsValid() );
    if( m_pDelivery )
    {
        switch( pEvent->GetEventType() )
        {
        case ET_BROADCAST:
            m_pDelivery->BroadcastDataPacket( reinterpret_cast< const ORA_VOID* >( pEvent ) );
            break;

        case ET_UNICAST:
            ORA_ASSERT( pEvent->GetSender() );
            m_pDelivery->UnicastDataPacket( pEvent->GetSender(), reinterpret_cast< const ORA_VOID* >( pEvent ) );
            break;

        case ET_MULTICAST:
            // TODO: wrapper the multicast interface
            break;

        case ET_TIMEOUT:
            // TODO: internal timeout event.
            break;
        }
    }
}

/**
 * @brief get devices' wifi rssi
 * !!! TBD: if need cache RSSI, and always return one value.
 *
 * @return RSSI value
 */
ORA_INT32 CRoleManager::GetDeviceRSSI() const
{
    //NotifyEvent( FS_MSG_IPC_GET_RSSI() );
    //!TODO! sync invoked wait event here.
    return m_DeviceRSSI;
}

/**
 * @brief receive data packet from sender device
 *
 * @param sender    from which sent the data packet
 * @param pPacket   the data packet
 * @param size      the data packet's size
 */
ORA_VOID CRoleManager::RecvDataPacket( const DEVICE_ID_T &sender, const ORA_VOID *pPacket, ORA_SIZE size )
{
    const ROLE_EVENT *pEvent = reinterpret_cast< const ROLE_EVENT* >( pPacket );
    ORA_ASSERT( pEvent && pEvent->IsValid() );
    ORA_ASSERT( sender == pEvent->GetSender() );
    if( m_pCurrState )
        m_pCurrState->ProcessEvent( pEvent );
}

ORA_VOID CRoleManager::OnMsgProcedure( const _MSG_HEAD *pMsg )
{
    ORA_ASSERT( pMsg );
    switch( pMsg->GetMsgID() )
    {
//    case MT_IPC_GET_RSSI_RESP:
//        m_DeviceRSSI = reinterpret_cast< const FS_MSG_IPC_GET_RSSI_RESP* >( pMsg )->GetRSSI();
//        //!TODO! issue sync event here.
//        break;
    }
}
// END: CRoleManager
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEG: CRoleState
/**
 * @brief constructor
 *
 * @param pContext  the role manager context for inner state using.
 */
CRoleManager::CRoleState::CRoleState( CRoleManager *pContext )
{
    ORA_ASSERT( pContext );
    m_pContext = pContext;
}

/**
 * @brief destructor
 */
CRoleManager::CRoleState::~CRoleState()
{
    // Do nothing.
}

/**
 * @brief change the state to specified state
 *
 * @param state   RoleStateType value.
 * @param pParam  Allow the caller passes by a parameter to new state.
 * @param bForced force to deactive previous status
 * @note: if forced flag used, it perhaps break out the previous state's handling procedure.
 */
ORA_VOID CRoleManager::CRoleState::ChangeState( RoleStateType state, ORA_VOID *pParam /* = ORA_NULL */, ORA_BOOL bForced /* = ORA_FALSE */ )
{
    ORA_ASSERT( state < CRoleManager::RST_STATE_TYPE_COUNT );
    ORA_ASSERT( m_pContext );
    m_pContext->SetState( state, pParam, bForced );
}

/**
 * @brief Enter and activate current state, and allow pass by a parameter to this state.
 * it need be override by children class.
 *
 * @param pParam outside parameter which need be passed by to this state.
 */
ORA_VOID CRoleManager::CRoleState::Activate( ORA_VOID *pParam /* = ORA_NULL */ )
{
    // Do nothing.
}

/**
 * @brief Deactivate and leave current state
 * it need be override by children class.
 *
 * @param bForced force to interpret current status
 */
ORA_VOID CRoleManager::CRoleState::Deactivate( ORA_BOOL bForced /* = ORA_FALSE */ )
{
    // Do nothing.
}

/**
 * @brief process the received role event
 *
 * @param pEvent role event
 */
ORA_VOID CRoleManager::CRoleState::ProcessEvent( const ROLE_EVENT *pEvent )
{
    // Do nothing.
}

/**
 * @brief Handle timer timeout
 *
 * @param hTimer    timer handler
 * @param pContext  context of CNoRoleState
 */
ORA_VOID CRoleManager::CRoleState::TimeoutHandler( ORA_HTIMER hTimer, ORA_VOID *pContext )
{
    ORA_ASSERT( pContext );
    reinterpret_cast< CRoleState* >( pContext )->ProcessEvent( new REVENT_TIMEOUT() );
}
// END: CRoleState
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEG: CNoRoleState
/**
 * @brief constructor
 *
 * @param pContext  the role manager context for inner state using.
 */
CRoleManager::CNoRoleState::CNoRoleState( CRoleManager *pContext )
    : CRoleState( pContext )
{
    m_hTimer = ORA_NULL;
}

/**
 * @brief destructor
 */
CRoleManager::CNoRoleState::~CNoRoleState()
{
    // Do nothing.
}

/**
 * @brief Enter and activate current state, and allow pass by a parameter to this state.
 * it need be override by children class.
 *
 * @param pParam outside parameter which need be passed by to this state.
 */
ORA_VOID CRoleManager::CNoRoleState::Activate( ORA_VOID *pParam /* = ORA_NULL */ )
{
    ORA_ASSERT( m_hTimer == ORA_NULL );
    m_hTimer = ORACreateTimer( CRoleState::TimeoutHandler, this );
    ORASetTimer( m_hTimer, NO_ROLE_LEISURE_TIMEOUT );
    SendEvent( new REVENT_QUERY_MASTER_INFO( m_DeviceID ) );
}

/**
 * @brief Deactivate and leave current state
 * it need be override by children class.
 *
 * @param bForced force to interpret current status
 */
ORA_VOID CRoleManager::CNoRoleState::Deactivate( ORA_BOOL bForced /* = ORA_FALSE */ )
{
    ORADestroyTimer( m_hTimer );
    m_hTimer = ORA_NULL;
}

/**
 * @brief process the received role event
 *
 * @param pEvent role event
 */
ORA_VOID CRoleManager::CNoRoleState::ProcessEvent( const ROLE_EVENT *pEvent )
{
    ORA_ASSERT( pEvent );
    switch( pEvent->GetEventID() )
    {
    case REID_QUERY_MASTER_INFO:
        {
            const REVENT_QUERY_MASTER_INFO *pInfo = reinterpret_cast< const REVENT_QUERY_MASTER_INFO* >( pEvent );
            if( pInfo->GetSender() < m_DeviceID )
                ChangeState( RST_PRE_ROLE );
        }
        break;

    case REID_DEFINER_DETECTED:
        ChangeState( RST_PRE_ROLE );
        break;

    case REID_MASTER_DETECTED:
        {
            const MASTER_INFO *pMasterInfo = reinterpret_cast< const REVENT_MASTER_DETECTED* >( pEvent )->GetMasterInfo();
            if( pMasterInfo )
            {
                SaveMasterInfo( *pMasterInfo );
                delete pMasterInfo;
                ChangeState( RST_SLAVE );
            }
        }
        break;

    case REID_TIMER_TIMEOUT:
        ChangeState( RST_DEFINER );
        break;

    default:
        ORA_ASSERT(ORA_FALSE);
    }
}
// END: CNoRoleState
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEG: CPreRoleState
/**
 * @brief constructor
 *
 * @param pContext  the role manager context for inner state using.
 */
CRoleManager::CPreRoleState::CPreRoleState( CRoleManager *pContext )
    : CRoleState( pContext )
{
}

/**
 * @brief destructor
 */
CRoleManager::CPreRoleState::~CPreRoleState()
{
    // Do nothing.
}

/**
 * @brief Enter and activate current state, and allow pass by a parameter to this state.
 * it need be override by children class.
 *
 * @param pParam outside parameter which need be passed by to this state.
 */
ORA_VOID CRoleManager::CPreRoleState::Activate( ORA_VOID *pParam /* = ORA_NULL */ )
{
    ORA_ASSERT( m_hTimer == ORA_NULL );
    m_hTimer = ORACreateTimer( CRoleState::TimeoutHandler, this );
    ORASetTimer( m_hTimer, PRE_ROLE_LEISURE_TIMEOUT );
    SendEvent( new REVENT_QUERY_MASTER_INFO( m_DeviceID ) );
}

/**
 * @brief Deactivate and leave current state
 * it need be override by children class.
 *
 * @param bForced force to interpret current status
 */
ORA_VOID CRoleManager::CPreRoleState::Deactivate( ORA_BOOL bForced /* = ORA_FALSE */ )
{
    ORADestroyTimer( m_hTimer );
    m_hTimer = ORA_NULL;
}

/**
 * @brief process the received role event
 *
 * @param pEvent role event
 */
ORA_VOID CRoleManager::CPreRoleState::ProcessEvent( const ROLE_EVENT *pEvent )
{
    ORA_ASSERT( pEvent );
    switch( pEvent->GetEventID() )
    {
    case REID_SET_MASTER_INFO:
        {
            //!!! TODO !!!
            ORA_BOOL bApConnected = ORA_TRUE;
            // bApConnected = ConnectAP();
            if( bApConnected )
                ChangeState( RST_MASTER );
            else
                ChangeState( RST_DEFINER );
        }
        break;

    case REID_FETACH_AP_RSSI:
        //SendEvent( new REVENT_FETACH_AP_RSSI_RESP( GetApRSSI() ) );
        break;

    case REID_MASTER_DETECTED:
        {
            const REVENT_MASTER_DETECTED *pInfo = reinterpret_cast< const REVENT_MASTER_DETECTED* >( pEvent );
            //SaveMasterInfo( pInfo->GetMasterInfo() );
            ChangeState( RST_SLAVE );
        }
        break;

    case REID_TIMER_TIMEOUT:
        ChangeState( RST_NO_ROLE );
        break;

    default:
        ORA_ASSERT(ORA_FALSE);
    }
}
// END: CPreRoleState
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEG: CDefinerState
/**
 * @brief constructor
 *
 * @param pContext  the role manager context for inner state using.
 */
CRoleManager::CDefinerState::CDefinerState( CRoleManager *pContext )
    : CRoleState( pContext )
{
}

/**
 * @brief destructor
 */
CRoleManager::CDefinerState::~CDefinerState()
{
    // Do nothing.
}

/**
 * @brief Enter and activate current state, and allow pass by a parameter to this state.
 * it need be override by children class.
 *
 * @param pParam outside parameter which need be passed by to this state.
 */
ORA_VOID CRoleManager::CDefinerState::Activate( ORA_VOID *pParam /* = ORA_NULL */ )
{
    // Do nothing.
}

/**
 * @brief Deactivate and leave current state
 * it need be override by children class.
 *
 * @param bForced force to interpret current status
 */
ORA_VOID CRoleManager::CDefinerState::Deactivate( ORA_BOOL bForced /* = ORA_FALSE */ )
{
    // Do nothing.
}

/**
 * @brief process the received role event
 *
 * @param pEvent role event
 */
ORA_VOID CRoleManager::CDefinerState::ProcessEvent( const ROLE_EVENT *pEvent )
{
    // Do nothing.
}
// END: CDefinerState
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEG: CSlaveState
/**
 * @brief constructor
 *
 * @param pContext  the role manager context for inner state using.
 */
CRoleManager::CSlaveState::CSlaveState( CRoleManager *pContext )
    : CRoleState( pContext )
{
}

/**
 * @brief destructor
 */
CRoleManager::CSlaveState::~CSlaveState()
{
    // Do nothing.
}

/**
 * @brief Enter and activate current state, and allow pass by a parameter to this state.
 * it need be override by children class.
 *
 * @param pParam outside parameter which need be passed by to this state.
 */
ORA_VOID CRoleManager::CSlaveState::Activate( ORA_VOID *pParam /* = ORA_NULL */ )
{
    // Do nothing.
}

/**
 * @brief Deactivate and leave current state
 * it need be override by children class.
 *
 * @param bForced force to interpret current status
 */
ORA_VOID CRoleManager::CSlaveState::Deactivate( ORA_BOOL bForced /* = ORA_FALSE */ )
{
    // Do nothing.
}

/**
 * @brief process the received role event
 *
 * @param pEvent role event
 */
ORA_VOID CRoleManager::CSlaveState::ProcessEvent( const ROLE_EVENT *pEvent )
{
    // Do nothing.
}
// END: CSlaveState
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEG: CMasterState
/**
 * @brief constructor
 *
 * @param pContext  the role manager context for inner state using.
 */
CRoleManager::CMasterState::CMasterState( CRoleManager *pContext )
    : CRoleState( pContext )
{
}

/**
 * @brief destructor
 */
CRoleManager::CMasterState::~CMasterState()
{
    // Do nothing.
}

/**
 * @brief Enter and activate current state, and allow pass by a parameter to this state.
 * it need be override by children class.
 *
 * @param pParam outside parameter which need be passed by to this state.
 */
ORA_VOID CRoleManager::CMasterState::Activate( ORA_VOID *pParam /* = ORA_NULL */ )
{
    // Do nothing.
}

/**
 * @brief Deactivate and leave current state
 * it need be override by children class.
 *
 * @param bForced force to interpret current status
 */
ORA_VOID CRoleManager::CMasterState::Deactivate( ORA_BOOL bForced /* = ORA_FALSE */ )
{
    // Do nothing.
}

/**
 * @brief process the received role event
 *
 * @param pEvent role event
 */
ORA_VOID CRoleManager::CMasterState::ProcessEvent( const ROLE_EVENT *pEvent )
{
    // Do nothing.
}
// END: CMasterState
//////////////////////////////////////////////////////////////////////////////
