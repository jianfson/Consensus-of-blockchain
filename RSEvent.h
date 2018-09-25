#ifndef __RS_EVENT_H__
#define __RS_EVENT_H__

enum RoleEventID
{
    REID_SET_MASTER_INFO,
    REID_MASTER_DETECTED,
    REID_QUERY_MASTER_INFO,

    REID_DEFINER_DETECTED,
    REID_TIMER_TIMEOUT,

    REID_QUERY_RSSI_INFO,
    REID_QUERY_RSSI_INFO_RESP,
    REID_NOTIFY_DEFINER_ALIVE,

    REID_FETACH_AP_RSSI,
    REID_FETACH_AP_RSSI_RESP,
};

struct MASTER_INFO
{
    DEVICE_ID_T DeviceID;
    string      IPAddr;

    MASTER_INFO( ORA_UINT32 id, const ORA_CHAR *pIP )
    {
        DeviceID = id;
        IPAddr   = pIP;
    }

    MASTER_INFO()
    {
        DeviceID = 0;
    }


    MASTER_INFO& operator= ( const MASTER_INFO& info )
    {
        DeviceID = info.DeviceID;
        IPAddr   = info.IPAddr;
    }
};

/**
 * @name RSEventType role state event type
 * @{ */
enum RSEventType
{
    ET_BROADCAST,   ///< broadcast the event to mesh network
    ET_UNICAST,     ///< unicast the event to specified target device
    ET_MULTICAST,   ///< get the resp event from specified source device
    ET_TIMEOUT      ///< timeout event
};
/**  @} */

#define ROLE_EVENT_ID_FLAG  0x5EA7  ///< REVT - id flag for identifying if the data is a role event.
/**
 * @name ROLE_EVENT base role event structure
 * @{ */
struct _ORA_ALIGN( 1 ) ROLE_EVENT
{
// Properties
private:
    ORA_UINT16  IdFlag;     ///< always equal '0x5EA7' - id flag for identifying if the data is a role event.
    ORA_UINT16  Id;         ///< Event ID
    DEVICE_ID_T Sender;     ///< Sender's device id type
    ORA_UINT32  Type;       ///< Event???
    ORA_SIZE    DataSize;   ///< the event data's size, exclude the event header

// Construct
protected:
    ROLE_EVENT( ORA_UINT16 eId, DEVICE_ID_T sender, RSEventType evtType, ORA_SIZE datSize )
    {
        IdFlag   = ORA_UINT16_TO_BE( ROLE_EVENT_ID_FLAG );
        Id       = ORA_UINT16_TO_BE( eId );
        Sender   = ORA_UINT32_TO_BE( sender );
        Type     = ORA_UINT32_TO_BE( evtType );
        DataSize = ORA_UINT32_TO_BE( datSize - sizeof( ROLE_EVENT ) );
    }

// Getter & Setter
public:
    inline ORA_BOOL IsValid() const
    {
        return ORA_BE_TO_UINT16( IdFlag ) == ROLE_EVENT_ID_FLAG;
    }

    inline ORA_UINT16 GetEventID() const
    {
        return ORA_BE_TO_UINT16( Id );
    }

    inline DEVICE_ID_T GetSender() const
    {
        return ORA_BE_TO_UINT32( Sender );
    }

    inline RSEventType GetEventType() const
    {
        return static_cast< RSEventType >( ORA_BE_TO_UINT32( Type ) );
    }

    inline ORA_VOID SetDataSize( ORA_SIZE size )
    {
        DataSize = ORA_UINT32_TO_BE( size );
    }

    inline ORA_SIZE GetDataSize() const
    {
        return ORA_BE_TO_UINT32( DataSize );
    }
};
/**  @} */

struct REVENT_QUERY_MASTER_INFO : public ROLE_EVENT
{
// Construct
public:
    REVENT_QUERY_MASTER_INFO( DEVICE_ID_T sender )
        : ROLE_EVENT( REID_QUERY_MASTER_INFO, sender, ET_BROADCAST, sizeof( REVENT_QUERY_MASTER_INFO ) )
    {
        // Do nothing.
    }
};

struct REVENT_MASTER_DETECTED : public ROLE_EVENT
{
//properties
private:
    ORA_UINT32 DeviceID;
    ORA_CHAR   IpAddr[ IPADDR_LEN ];

// Getters & Setters
public:
    const MASTER_INFO* GetMasterInfo() const
    {
        MASTER_INFO *pInfo = new MASTER_INFO(DeviceID, IpAddr);
        return pInfo;
    }
};

struct REVENT_TIMEOUT: public ROLE_EVENT
{
// Construct
public:
    REVENT_TIMEOUT()
        : ROLE_EVENT( REID_TIMER_TIMEOUT, 0, ET_TIMEOUT, sizeof( REVENT_TIMEOUT ) )
    {
        // Do nothing.
    }
};


#endif /* __RS_EVENT_H__ */
