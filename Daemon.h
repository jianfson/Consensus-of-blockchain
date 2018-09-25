#ifndef __FS_DAEMON_H__
#define __FS_DAEMON_H__

#include "CommService.h"
#include "Network.h"
#include "Profile.h"
#include "IPCCtrl.h"
#include "RoleState.h"

/**
 * @name CDaemon the daemon process for FastSetup
 * @{ */
class CDaemon : public CCommService
{
// Constructor & Destructor
public:
    CDaemon();
    ~CDaemon();

// Operations
public:
    /**
     * @brief mainly thread for fast setup daemon, and pending for ever.
     */
    ORA_VOID WaitForExit();

// Overrides( PUBLIC )
public:
    /**
     * @brief start the fast setup daemon
     *
     * @return ORA_TRUE if start successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL Start();

    /**
     * @brief stop the fast setup daemon
     */
    ORA_VOID Stop();

    /**
     * @brief Connect the external network via AP connection.
     * @note the function should be used under Master Status.
     */
    inline ORA_VOID ConnectExternalNetwork()
    {
//        ORA_ASSERT( m_pNwSrv );
        m_pNwSrv->ConnectExternalNetwork();
    }

    /**
     * @brief get AP connection status for current device
     *
     * @return AP connection status:
     */
    inline NwConnStat GetAPConnStatus()
    {
//        ORA_ASSERT( m_pNwSrv );
        return m_pNwSrv->GetAPConnStatus();
    }


// Overrides( PRIVATE )
private:
    ORA_VOID OnMsgProcedure( const _MSG_HEAD *pMsg );

// Properties
private:
    CNetworkService *m_pNwSrv;
    CProfile        *m_pConfig;
    CIPCController  *m_pIPCCtrl;
    ORA_HEVENT       m_hWaitingForExit;
    ORA_UINT64       m_DeviceID;
    CRoleManager    *m_pRoleManager;

    mutable ORA_CRITICAL_SECTION m_PropLock;
};
/**  @} */

#endif /* __FS_DAEMON_H__ */
