#ifndef _ROLE_STATE_H_
#define _ROLE_STATE_H_

#include "Network.h"
#include "Profile.h"
#include "CommService.h"
#include "RSEvent.h"

#include <map>

using namespace std;

class CRoleManager : public INwDataReceiver, public CCommService
{
// Assistant Structure
public:
    enum RoleStateType
    {
        RST_NONE,
        RST_NO_ROLE,
        RST_DEFINER,
        RST_PRE_ROLE,
        RST_SLAVE,
        RST_MASTER,

        RST_STATE_TYPE_COUNT    ///< the RoleState Type's total amount
    };

// Inner role state class for role manager.
protected:
    /**
     * @name CRoleState the base class for role state.
     * @{ */
    class CRoleState
    {
    // Constructor & Destructor
    public:
        /**
         * @brief constructor
         *
         * @param pContext  the role manager context for inner state using.
         */
        CRoleState( CRoleManager *pContext );
        /**
         * @brief destructor
         */
        virtual ~CRoleState();

    // Operations
    public:
        /**
         * @brief change the state to specified state
         *
         * @param state   RoleStateType value.
         * @param pParam  Allow the caller passes by a parameter to new state.
         * @param bForced force to deactive previous status
         * @note: if forced flag used, it perhaps break out the previous state's handling procedure.
         */
        ORA_VOID ChangeState( RoleStateType state, ORA_VOID *pParam = ORA_NULL, ORA_BOOL bForced = ORA_FALSE );

    // Overrides
    public:
        /**
         * @brief Enter and activate current state, and allow pass by a parameter to this state.
         * it need be override by children class.
         *
         * @param pParam outside parameter which need be passed by to this state.
         */
        virtual ORA_VOID Activate( ORA_VOID *pParam = ORA_NULL ) = 0;

        /**
         * @brief Deactivate and leave current state
         * it need be override by children class.
         *
         * @param bForced force to deactive previous status
         */
        virtual ORA_VOID Deactivate( ORA_BOOL bForced = ORA_FALSE ) = 0;

        /**
         * @brief process the received role event
         *
         * @param pEvent role event
         */
        virtual ORA_VOID ProcessEvent( const ROLE_EVENT *pEvent ) = 0;

        /**
         * @brief return current status' type.
         *
         * @return role state type
         */
        virtual RoleStateType GetStateType() const = 0;

    // Assistant
    protected:
        /**
         * @brief send the event the target devices via broadcast/unicast/multicast approach, or tigger internal timeout event.
         * @note (TBD: or security transfer - TCP)
         * @param pEvent the event data
         */
        inline ORA_VOID SendEvent( const ROLE_EVENT *pEvent )
        {
            ORA_ASSERT( m_pContext );
            ORA_ASSERT( pEvent );
            m_pContext->SendEvent( pEvent );
        }

        /**
         * @brief save master's information to role manager
         *
         * @param pInfo Master's information
         */
        inline ORA_VOID SaveMasterInfo( const MASTER_INFO& info )
        {
            ORA_ASSERT( m_pContext );
            m_pContext->SaveMasterInfo( info );
        }

        /**
         * @brief Get master's information
         *
         * @return MASTER_INFO handler
         */
        inline MASTER_INFO& GetMasterInfo() const
        {
            ORA_ASSERT( m_pContext );
            return m_pContext->GetMasterInfo();
        }

        inline ORA_INT32 GetApRSSI() const
        {
            ORA_ASSERT( m_pContext );
            return m_pContext->GetDeviceRSSI();
        }

    // Callbacks
    public:
        /**
         * @brief Handle timer timeout
         *
         * @param hTimer    timer handler
         * @param pContext  context of CNoRoleState
         */
        static ORA_VOID TimeoutHandler( ORA_HTIMER hTimer, ORA_VOID *pContext );

    // Properties
    private:
        CRoleManager *m_pContext;      ///< Role Manager handler

    protected:
        DEVICE_ID_T   m_DeviceID;      ///< Device UUID
        ORA_HTIMER    m_hTimer;        ///< Timer handler for timeout event
    };
    /**  @} */

    /**
     * @name CNoRoleState the base class for role state.
     * @{ */
    class CNoRoleState : public CRoleState
    {
    // Assistant definition
    private:
        #define NO_ROLE_LEISURE_TIMEOUT     8 * 1000
        #define PRE_ROLE_LEISURE_TIMEOUT    8 * 1000
        #define DEFINER_LEISURE_TIMEOUT     8 * 1000
        #define MASTER_HEAT_BEAT_TIMEOUT    8 * 1000

    // Constructor & Destructor
    public:
        /**
         * @brief constructor
         *
         * @param pContext  the role manager context for inner state using.
         */
        CNoRoleState( CRoleManager *pContext );
        /**
         * @brief destructor
         */
        ~CNoRoleState();

    // Overrides
    public:
        /**
         * @brief Enter and activate current state, and allow pass by a parameter to this state.
         * it need be override by children class.
         *
         * @param pParam outside parameter which need be passed by to this state.
         */
        virtual ORA_VOID Activate( ORA_VOID *pParam = ORA_NULL );

        /**
         * @brief Deactivate and leave current state
         * it need be override by children class.
         *
         * @param bForced force to deactive previous status
         */
        virtual ORA_VOID Deactivate( ORA_BOOL bForced = ORA_FALSE );

        /**
         * @brief process the received role event
         *
         * @param pEvent role event
         */
        virtual ORA_VOID ProcessEvent( const ROLE_EVENT *pEvent );

        /**
         * @brief return current status' type.
         *
         * @return role state type
         */
        virtual RoleStateType GetStateType() const
        {
            return RST_NO_ROLE;
        }

    // Callbacks
    private:
        /**
         * @brief Handle timer timeout
         *
         * @param hTimer    timer handler
         * @param pContext  context of CNoRoleState
         */
        static ORA_VOID TimerHandler( ORA_HTIMER hTimer, ORA_VOID *pContext );
    };
    /**  @} */

    /**
     * @name CPreRoleState the base class for role state.
     * @{ */
    class CPreRoleState : public CRoleState
    {
    // Constructor & Destructor
    public:
        /**
         * @brief constructor
         *
         * @param pContext  the role manager context for inner state using.
         */
        CPreRoleState( CRoleManager *pContext );
        /**
         * @brief destructor
         */
        ~CPreRoleState();

    // Overrides
    public:
        /**
         * @brief Enter and activate current state, and allow pass by a parameter to this state.
         * it need be override by children class.
         *
         * @param pParam outside parameter which need be passed by to this state.
         */
        virtual ORA_VOID Activate( ORA_VOID *pParam = ORA_NULL );

        /**
         * @brief Deactivate and leave current state
         * it need be override by children class.
         *
         * @param bForced force to deactive previous status
         */
        virtual ORA_VOID Deactivate( ORA_BOOL bForced = ORA_FALSE );

        /**
         * @brief process the received role event
         *
         * @param pEvent role event
         */
        virtual ORA_VOID ProcessEvent( const ROLE_EVENT *pEvent );

        /**
         * @brief return current status' type.
         *
         * @return role state type
         */
        virtual RoleStateType GetStateType() const
        {
            return RST_PRE_ROLE;
        }
    };
    /**  @} */

    /**
     * @name CDefinerState the base class for role state.
     * @{ */
    class CDefinerState : public CRoleState
    {
    // Constructor & Destructor
    public:
        /**
         * @brief constructor
         *
         * @param pContext  the role manager context for inner state using.
         */
        CDefinerState( CRoleManager *pContext );
        /**
         * @brief destructor
         */
        ~CDefinerState();

    // Overrides
    public:
        /**
         * @brief Enter and activate current state, and allow pass by a parameter to this state.
         * it need be override by children class.
         *
         * @param pParam outside parameter which need be passed by to this state.
         */
        virtual ORA_VOID Activate( ORA_VOID *pParam = ORA_NULL );

        /**
         * @brief Deactivate and leave current state
         * it need be override by children class.
         *
         * @param bForced force to deactive previous status
         */
        virtual ORA_VOID Deactivate( ORA_BOOL bForced = ORA_FALSE );

        /**
         * @brief process the received role event
         *
         * @param pEvent role event
         */
        virtual ORA_VOID ProcessEvent( const ROLE_EVENT *pEvent );

        /**
         * @brief return current status' type.
         *
         * @return role state type
         */
        virtual RoleStateType GetStateType() const
        {
            return RST_DEFINER;
        }
    };
    /**  @} */

    /**
     * @name CSlaveState the base class for role state.
     * @{ */
    class CSlaveState : public CRoleState
    {
    // Constructor & Destructor
    public:
        /**
         * @brief constructor
         *
         * @param pContext  the role manager context for inner state using.
         */
        CSlaveState( CRoleManager *pContext );
        /**
         * @brief destructor
         */
        ~CSlaveState();

    // Overrides
    public:
        /**
         * @brief Enter and activate current state, and allow pass by a parameter to this state.
         * it need be override by children class.
         *
         * @param pParam outside parameter which need be passed by to this state.
         */
        virtual ORA_VOID Activate( ORA_VOID *pParam = ORA_NULL );

        /**
         * @brief Deactivate and leave current state
         * it need be override by children class.
         *
         * @param bForced force to deactive previous status
         */
        virtual ORA_VOID Deactivate( ORA_BOOL bForced = ORA_FALSE );

        /**
         * @brief process the received role event
         *
         * @param pEvent role event
         */
        virtual ORA_VOID ProcessEvent( const ROLE_EVENT *pEvent );

        /**
         * @brief return current status' type.
         *
         * @return role state type
         */
        virtual RoleStateType GetStateType() const
        {
            return RST_SLAVE;
        }
    };
    /**  @} */

    /**
     * @name CMasterState the base class for role state.
     * @{ */
    class CMasterState : public CRoleState
    {
    // Constructor & Destructor
    public:
        /**
         * @brief constructor
         *
         * @param pContext  the role manager context for inner state using.
         */
        CMasterState( CRoleManager *pContext );
        /**
         * @brief destructor
         */
        ~CMasterState();

    // Overrides
    public:
        /**
         * @brief Enter and activate current state, and allow pass by a parameter to this state.
         * it need be override by children class.
         *
         * @param pParam outside parameter which need be passed by to this state.
         */
        virtual ORA_VOID Activate( ORA_VOID *pParam = ORA_NULL );

        /**
         * @brief Deactivate and leave current state
         * it need be override by children class.
         *
         * @param bForced force to deactive previous status
         */
        virtual ORA_VOID Deactivate( ORA_BOOL bForced = ORA_FALSE );

        /**
         * @brief process the received role event
         *
         * @param pEvent role event
         */
        virtual ORA_VOID ProcessEvent( const ROLE_EVENT *pEvent );

        /**
         * @brief return current status' type.
         *
         * @return role state type
         */
        virtual RoleStateType GetStateType() const
        {
            return RST_MASTER;
        }
    };
    /**  @} */

// Constructor & Destructor
public:

    /**
     * @brief constructor for CRoleManager
     *
     * @param pDelivery the network data delivery interface.
     */
    CRoleManager( INwDataDelivery* pDelivery );

    /**
     * @brief destructor for CRoleManager
     */
    ~CRoleManager();


// Operations
public:
    /**
     * @brief set current state to specified state
     *
     * @param state   RoleStateType value.
     * @param pParam  Allow the caller passes by a parameter to new state.
     * @param bForced force to deactive previous status
     * @note: if forced flag used, it perhaps break out the previous state's handling procedure.
     */
    ORA_VOID SetState( RoleStateType state, ORA_VOID *pParam = ORA_NULL, ORA_BOOL bForced = ORA_FALSE );

    /**
     * @brief send the event the target devices via broadcast/unicast/multicast approach, or tigger internal timeout event.
     * @note (TBD: or security transfer - TCP)
     * @param pEvent the event data
     */
    ORA_VOID SendEvent( const ROLE_EVENT *pEvent );

    /**
     * @brief get devices' wifi rssi
     * !!! TBD: if need cache RSSI, and always return one value.
     *
     * @return RSSI value
     */
    ORA_INT32 GetDeviceRSSI() const;

    /**
     * @brief get current state enumaration value.
     *
     * @return RoleStateType value.
     */
    inline RoleStateType CurrentState() const
    {
        return m_pCurrState ? m_pCurrState->GetStateType() : RST_NONE;
    }

    /**
     * @brief save master's information to role manager
     *
     * @param pInfo Master's information
     */
    inline ORA_VOID SaveMasterInfo( const MASTER_INFO& info )
    {
        m_MasterInfo = info;
    }

    /**
     * @brief Get master's information
     *
     * @return MASTER_INFO handler
     */
    inline MASTER_INFO& GetMasterInfo() const
    {
        return m_MasterInfo;
    }

// Overrides
public:
    /**
     * @brief start the role manager
     *
     * @return ORA_TRUE if start successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL Start();

    /**
     * @brief stop the role manager
     */
    ORA_VOID Stop();

    /**
     * @brief receive data packet from sender device
     *
     * @param sender    from which sent the data packet
     * @param pPacket   the data packet
     * @param size      the data packet's size
     */
    ORA_VOID RecvDataPacket( const DEVICE_ID_T &sender, const ORA_VOID *pPacket, ORA_SIZE size );

// Overrides
private:
    ORA_VOID OnMsgProcedure( const _MSG_HEAD *pMsg );

// Thread routines
private:
    // TODO: using thread-mechnism to cache the event, and exit the callback call rapidly.
    // it can make RecvDataPacket() responding higher effectively.
    // static ORA_INT_PTR ListenEventThread( ORA_VOID* pContext );

// Properties
private:
    typedef std::map< RoleStateType, CRoleState* > CRoleStateMap;

    INwDataDelivery *m_pDelivery;               ///< deliver the data to other network device
    CRoleState      *m_pCurrState;              ///< current role state handler
    CRoleStateMap    m_RoleStateMap;            ///< A map container to hold all available role state instance
    ORA_INT32        m_DeviceRSSI;

    mutable MASTER_INFO m_MasterInfo;

// TODO: performance improve...
//    ORA_HTHREAD      m_hListenEventThread;
//    mutable ORA_CRITICAL_SECTION m_EventLock;
};
#endif
