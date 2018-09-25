#ifndef __IPC_CONTROLLER_H__
#define __IPC_CONTROLLER_H__

//#include "ora_ipc_module_fastsetup.h" ///< Fast Setup IPC Message Definitions
#include "ora_ipc.h"
#include "CommService.h"

class CDaemon;
/**
 * @name CIPCController the controller for handlign communcation with IPC message
 * @note singlethon instance
 *
 * @{ */
class CIPCController : public CCommService
{
// Constructor & Destructor
private:
    /**
     * @brief Constructor for CIPCController
     */
    CIPCController();

    /**
     * @brief Destructor for CIPCController
     */
    ~CIPCController();

// Instance
public:
    static CIPCController* GetInstance()
    {
        static CIPCController s_ipcCtrl;
        return &s_ipcCtrl;
    }

// Operations
public:
    /**
     * @brief Initialize the IPC Controller, register to IPC daemon.
     *
     * @return ORA_TRUE if initialized successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL Start();

    /**
     * @brief Uninitialize the IPC Controller, unregister from IPC daemon.
     */
    ORA_VOID Stop();

// Operations
public:
    /**
     * @brief Send the request message to target IPC module
     * @note it need covert the internal Message structure to IPC acceptable format
     *
     * @param target target IPC module
     * @param pMsg   the message data need be sent.
     *
     * @return ORA_TRUE if send successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL SendMessage( ORA_UINT32 target, const ORA_VOID *pMsg );

// Callbacks
private:
    /**
     * @brief callback function prototype for incoming event handling
     * @param sender sender module name of the event
     * @param event  event name
     * @param value  event data
    **/
    static ORA_VOID EventProcess(const ORA_CHAR *sender, const ORA_CHAR *event, const ORA_IPC_MSG *value);

    /**
     * @brief callback function prototype for incoming request handling
     * @param sender sender module name of the request
     * @param msgid  request name
     * @param value  request data
     */
    static ORA_VOID RequestMsgProcess(const ORA_CHAR *sender, const ORA_CHAR *msgid, const ORA_IPC_MSG *value);

    /**
     * @brief callback function prototype for incoming reply handling
     * @param sender sender module name of the reply
     * @param msgid  reply name
     * @param value  reply data
     **/
    static ORA_VOID ReplyMsgProcess(const ORA_CHAR *sender, const ORA_CHAR *msgid, const ORA_IPC_MSG *value);

// Overrides
private:
    ORA_VOID OnMsgProcedure( const _MSG_HEAD *pMsg );

// Properties
private:
    CDaemon      *m_pDaemon;       ///< the fast setup daemon handler
};

class IDataDelivery
{
public:
    IDataDelivery();
    ~IDataDelivery();
    ORA_VOID SendData();

};

/**  @} */

#endif /* __IPC_CONTROLLER_H__ */
