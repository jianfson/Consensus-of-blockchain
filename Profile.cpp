#include "Base.h"
#include "Profile.h"
#include "ora_sys.h"

#define FS_PROFILE_NAME "fast_setup.conf"

const ORA_CHAR *CONF_KEY_USER_ID             = "USER_ID";             ///< User ID for initializing the public mesh ESSID.
const ORA_CHAR *CONF_KEY_GROUP_ID            = "GROUP_ID";            ///< Group ID for initializing the private mesh ESSID.
const ORA_CHAR *CONF_KEY_PUB_MESH            = "PUBLIC_MESH";         ///< The Public Mesh params String array with following fixed format [ESSID, CHANNEL, SUBMASK, IPADDR]
const ORA_CHAR *CONF_KEY_PRV_MESH            = "PRIVATE_MESH";        ///< The Private Mesh params String array with following fixed format [ESSID, CHANNEL, SUBMASK, IPADDR]
const ORA_CHAR *CONF_KEY_SCANNING_INTERVAL   = "SCANNING_INTERVAL";   ///< The Scanning interval time for finding neighbors on the Public Mesh network
const ORA_CHAR *CONF_KEY_VISIBLE_INTERVAL    = "VISIBLE_INTERVAL";    ///< The Visible interval time to provide Private Mesh info & AP querying for other device.
const ORA_CHAR *CONF_KEY_DEVICE_ID           = "DEVICE_ID";           ///< Device series number
const ORA_CHAR *CONF_KEY_AP_SSID_SERIES      = "AP_SSID_SERIES";      ///< AP SSID name list, Note: the <SSID, KeyMgmnt, Password> must be a pair.
const ORA_CHAR *CONF_KEY_AP_KEY_MGMNT_SERIES = "AP_KEY_MGMNT_SERIES"; ///< AP Key management list, Note: the <SSID, KeyMgmnt, Password> must be a pair.
const ORA_CHAR *CONF_KEY_AP_PWD_SERIES       = "AP_PWD_SERIES";       ///< AP Password list, Note: the <SSID, KeyMgmnt, Password> must be a pair.

/**
 * @brief CProfile's constructor
 *
 * @note load the ora_config module for saving/reading profile data
 */
CProfile::CProfile()
{
    m_pConf            = ORA_NULL;
    m_UserID           = 0;
    m_GroupID          = 0;
    m_ScanningInterval = 0;
    m_VisibleInterval  = 0;

    LoadConfiguration();
}

/**
 * @brief CProfile's destructor
 *
 * @note unload the ora_config module
 */
CProfile::~CProfile()
{
    ORA_ASSERT( m_pConf );
    ora_config_unload( m_pConf );
}

/**
 * @brief Load all profiles' data
 */
ORA_VOID CProfile::LoadConfiguration()
{
    m_pConf = ora_config_load( FS_PROFILE_NAME );
    ORA_ASSERT( m_pConf );

    // Get User ID
    if( !ora_config_read_int32( m_pConf, CONF_KEY_USER_ID, &m_UserID ) )
        ora_config_write_int32( m_pConf, CONF_KEY_USER_ID, m_UserID );

    // Get Group ID
    if( !ora_config_read_int32( m_pConf, CONF_KEY_GROUP_ID, &m_GroupID ) )
        ora_config_write_int32( m_pConf, CONF_KEY_GROUP_ID, m_GroupID );

    // Get Scanning Interval
    if( !ora_config_read_int32( m_pConf, CONF_KEY_SCANNING_INTERVAL, &m_ScanningInterval ) )
        ora_config_write_int32( m_pConf, CONF_KEY_SCANNING_INTERVAL, m_ScanningInterval );

    // Get Visible Interval
    if( !ora_config_read_int32( m_pConf, CONF_KEY_VISIBLE_INTERVAL, &m_VisibleInterval ) )
        ora_config_write_int32( m_pConf, CONF_KEY_VISIBLE_INTERVAL, m_VisibleInterval );

    // Get Public Mesh Info
    m_PublicMeshInfo = ReadMeshInfo( CONF_KEY_PUB_MESH );

    // Get Private Mesh Info
    m_PrivMeshInfo = ReadMeshInfo( CONF_KEY_PRV_MESH );

    // Get AP List Info
    ReadApInfoList( &m_ApInfoList );

    // Get Device ID
    if( !ora_config_read_int64( m_pConf, CONF_KEY_DEVICE_ID, reinterpret_cast< ORA_INT64* >( &m_DeviceID ) ) )
    {
        const ORA_CHAR *pDeviceID = get_device_id();
        if( pDeviceID )
        {
            sscanf( pDeviceID, "%llx", &m_DeviceID );
            delete pDeviceID;
        }
        ora_config_write_int64( m_pConf, CONF_KEY_DEVICE_ID, static_cast< ORA_INT64 >( m_DeviceID ) );
    }
}

/**
 * @brief Set User ID for public mesh network
 *
 * @param gid  User ID
 *
 * @return ORA_TRUE if set successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::SetUserID( ORA_INT32 id )
{
    ORA_ASSERT( m_pConf );
    if( ora_config_write_int32( m_pConf, CONF_KEY_USER_ID, id ) &&
            ora_config_save( m_pConf ) )
    {
        m_UserID = id;
        return ORA_TRUE;
    }

    return ORA_FALSE;
}

/**
 * @brief Set Group ID for private mesh network
 *
 * @param gid Group ID
 *
 * @return ORA_TRUE if set successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::SetGroupID( ORA_INT32 gid )
{
    ORA_ASSERT( m_pConf );
    if( ora_config_write_int32( m_pConf, CONF_KEY_GROUP_ID, gid ) &&
            ora_config_save( m_pConf ) )
    {
        m_GroupID = gid;
        return ORA_TRUE;
    }

    return ORA_FALSE;
}

/**
 * @brief AddApInfo add AP information to profile file
 *
 * @param apInfo   AP_INFO data
 *
 * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::AddApInfo( const AP_INFO *apInfo )
{
    m_ApInfoList.push_back( *apInfo );
    return ORA_TRUE;
}

/**
 * @brief Set the scanning interval item (second)
 *
 * @param interval Scanning interval time
 *
 * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::SetScanningInterval( ORA_INT32 interval )
{
    ORA_ASSERT( m_pConf );
    if( ora_config_write_int32( m_pConf, CONF_KEY_SCANNING_INTERVAL, interval ) &&
            ora_config_save( m_pConf ) )
    {
        m_ScanningInterval = interval;
        return ORA_TRUE;
    }

    return ORA_FALSE;
}

/**
 * @brief Set the visible interval item (second)
 *
 * @param interval visible interval time
 *
 * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::SetVisibleInterval( ORA_INT32 interval )
{
    ORA_ASSERT( m_pConf );
    if( ora_config_write_int32( m_pConf, CONF_KEY_VISIBLE_INTERVAL, interval ) &&
            ora_config_save( m_pConf ) )
    {
        m_VisibleInterval = interval;
        return ORA_TRUE;
    }

    return ORA_FALSE;
}

/**
 * @brief Set Private Mesh Information
 *
 * @param pInfo Mesh Information
 *
 * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::SetPrivMeshInfo( const MESH_INFO *pInfo )
{
    m_PrivMeshInfo = *pInfo;
    return WriteMeshInfo( CONF_KEY_PRV_MESH, pInfo );
}

/**
 * @brief Set Public Mesh Information
 *
 * @param pInfo Mesh Information
 *
 * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::SetPublicMeshInfo( const MESH_INFO *pInfo )
{
    m_PublicMeshInfo = *pInfo;
    return WriteMeshInfo( CONF_KEY_PUB_MESH, pInfo );
}

/**
 * @brief Read mesh info from Profile file
 *
 * @param pKey  Public/Private Mesh Info Key Item
 *
 * @return MESH_INFO data
 */
MESH_INFO CProfile::ReadMeshInfo( const ORA_CHAR *pKey )
{
    MESH_INFO  info;
    ORA_CHAR  *pBuff = ORA_NULL;
    ORA_INT32  dataCnt;
    if( ora_config_read_string_array( m_pConf, pKey, &pBuff, &dataCnt ) )
    {
        if( pBuff )
        {
            ORA_INT32 offset = 0;
            info.ESSID = &pBuff[ offset ];

            offset += strlen( pBuff ) + 1;
            info.SubMask = &pBuff[ offset ];

            offset += strlen( pBuff ) + 1;
            info.IpAddr = &pBuff[ offset ];

            offset += strlen( pBuff ) + 1;
            info.Channel = atoi( &pBuff[ offset ] );

            free( pBuff );
        }
    }

    return info;
}

/**
 * @brief Write mesh info to Profile file
 *
 * @param pKey  Public/Private Mesh Info Key Item
 * @param pInfo  Mesh Info data need to be saved
 *
 * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::WriteMeshInfo( const ORA_CHAR *pKey, const MESH_INFO *pInfo )
{
    return ORA_FALSE;
}

/** * @brief Read AP Info List from profile file.  *
 * @param pInfoList the AP_INFO vector which carried the returned value
 *
 * @return ORA_TRUE if read successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::ReadApInfoList( CApInfoList *pInfoList )
{
    return ORA_FALSE;
}

/**
 * @brief Write AP Info List to profile file.
 *
 * @param pInfoList the AP_INFO vector which need be written
 *
 * @return ORA_TRUE if write successfully, otherwise return ORA_FALSE
 */
ORA_BOOL CProfile::WriteApInfoList( const CApInfoList *pInfoList )
{
    return ORA_FALSE;
}
