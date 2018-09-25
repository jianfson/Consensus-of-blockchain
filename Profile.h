#ifndef __FAST_SETUP_PROFILE__
#define __FAST_SETUP_PROFILE__

#include "ora_config.h"
#include <string>
#include <vector>

using namespace std;

extern const ORA_CHAR *CONF_KEY_USER_ID;
extern const ORA_CHAR *CONF_KEY_GROUP_ID;
extern const ORA_CHAR *CONF_KEY_PUB_MESH;
extern const ORA_CHAR *CONF_KEY_PRV_MESH;
extern const ORA_CHAR *CONF_KEY_SCANNING_INTERVAL;
extern const ORA_CHAR *CONF_KEY_VISIBLE_INTERVAL;
extern const ORA_CHAR *CONF_KEY_DEVICE_ID;
extern const ORA_CHAR *CONF_KEY_AP_SSID_SERIES;
extern const ORA_CHAR *CONF_KEY_AP_KEY_MGMNT_SERIES;
extern const ORA_CHAR *CONF_KEY_AP_PWD_SERIES;

/**
 * @name CProfile to read/save FastSetup relative configuration info
 *
 * @{ */
class CProfile
{
// Constructor & Destructor
private:
    CProfile();
    ~CProfile();

// Instance
public:
    /**
     * @brief Instance method for singlethon pattern
     *
     * @return CProfile handler.
     */
    static CProfile* GetInstance()
    {
        static CProfile s_Profile;
        return &s_Profile;
    }

// Operations
public:
    /**
     * @brief Get User ID for public mesh network
     *
     * @return User ID
     */
    inline ORA_INT32 GetUserID() const
    {
        return m_UserID;
    }

    /**
     * @brief Set User ID for public mesh network
     *
     * @param id  User ID
     *
     * @return ORA_TRUE if set successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL SetUserID( ORA_INT32 id );

    /**
     * @brief Get Group ID for private mesh network
     *
     * @return Group ID
     */
    inline ORA_INT32 GetGroupID() const
    {
        return m_GroupID;
    }

    /**
     * @brief Set Group ID for private mesh network
     *
     * @param gid  Group ID
     *
     * @return ORA_TRUE if set successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL SetGroupID( ORA_INT32 gid );

    /**
     * @brief GetApInfoList get AP information list from profile file
     *
     * @return AP Info list
     */

    inline CApInfoList GetApInfoList() const
    {
        return m_ApInfoList;
    }

    /**
     * @brief AddApInfo add AP information to profile file
     *
     * @param apInfo   AP_INFO data
     *
     * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL AddApInfo( const AP_INFO *apInfo );

    /**
     * @brief Get the scanning interval time (second)
     * @note scanning interval time will be used under initial public mesh state
     *
     * @return Scanning interval time
     */
    inline ORA_INT32 GetScanningInterval() const
    {
        return m_ScanningInterval;
    }

    /**
     * @brief Set the scanning interval item (second)
     *
     * @param interval Scanning interval time
     *
     * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL SetScanningInterval( ORA_INT32 interval );

    /**
     * @brief Get the visible interval time (second)
     * @note visible interval time will be used under initial public mesh state
     *
     * @return visible interval time
     */
    inline ORA_INT32 GetVisibleInterval() const
    {
        return m_VisibleInterval;
    }

    /**
     * @brief Set the visible interval item (second)
     *
     * @param interval visible interval time
     *
     * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL SetVisibleInterval( ORA_INT32 interval );

    /**
     * @brief Get private mesh network information
     *
     * @return MESH_INFO of private mesh
     */
    inline MESH_INFO GetPrivMeshInfo() const
    {
        return m_PrivMeshInfo;
    }

    /**
     * @brief Set Private Mesh Information
     *
     * @param pInfo Mesh Information
     *
     * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL SetPrivMeshInfo( const MESH_INFO *pInfo );

    /**
     * @brief Get public mesh network information
     *
     * @return MESH_INFO of private mesh
     */
    inline MESH_INFO GetPublicMeshInfo() const
    {
        return m_PublicMeshInfo;
    }

    /**
     * @brief Set Public Mesh Information
     *
     * @param pInfo Mesh Information
     *
     * @return ORA_TRUE if add successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL SetPublicMeshInfo( const MESH_INFO *pInfo );

    /**
     * @brief Get the Device ID (UUID)
     *
     * @return Device Series Number (ID)
     */
    inline ORA_UINT64 GetDeviceID() const
    {
        return m_DeviceID;
    }

// Assistants
private:
    /**
     * @brief Loads all profiles' data
     */
    ORA_VOID LoadConfiguration();

    /**
     * @brief Read mesh info from Profile file
     *
     * @param pKey  Public/Private Mesh Info Key Item
     *
     * @return MESH_INFO data
     */
    MESH_INFO ReadMeshInfo( const ORA_CHAR *pKey );

    /**
     * @brief Write mesh info to Profile file
     *
     * @param pKey  Public/Private Mesh Info Key Item
     * @param pInfo  Mesh Info data need to be saved
     *
     * @return ORA_TRUE if write successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL WriteMeshInfo( const ORA_CHAR *pKey, const MESH_INFO *pInfo );

    /**
     * @brief Read AP Info List from profile file.
     *
     * @param pInfoList the AP_INFO vector which carried the returned value
     *
     * @return ORA_TRUE if read successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL ReadApInfoList( CApInfoList *pInfoList );

    /**
     * @brief Write AP Info List to profile file.
     *
     * @param pInfoList the AP_INFO vector which need be written
     *
     * @return ORA_TRUE if write successfully, otherwise return ORA_FALSE
     */
    ORA_BOOL WriteApInfoList( const CApInfoList *pInfoList );

// Properties
private:
    typedef vector< string >    CApSSIDList;
    typedef vector< ORA_INT32 > CApKeyMgmntList;
    typedef vector< string >    CApPasswordList;

    ora_config_t    *m_pConf;            ///< the ora_config handler for data saving/reading
    ORA_INT32        m_UserID;           ///< User ID for public mesh network
    ORA_INT32        m_GroupID;          ///< Group ID for private mesh network
    ORA_INT32        m_ScanningInterval; ///< Scanning Interval time for initial public mesh state
    ORA_INT32        m_VisibleInterval;  ///< Visible Interval time for configured public mesh state which make device is visible when other neighbors are  scanning.
    MESH_INFO        m_PrivMeshInfo;     ///< Private mesh network information
    MESH_INFO        m_PublicMeshInfo;   ///< Public mesh network information
    CApInfoList      m_ApInfoList;       ///< AP Info list for current device
    CApSSIDList      m_SSIDList;         ///< save the AP SSIDs
    CApKeyMgmntList  m_KeyMgmntList;     ///< save the AP Key management type: 0, 1, 2
    CApPasswordList  m_PasswordList;     ///< save the AP password
    ORA_UINT64       m_DeviceID;         ///< Deivce's Series Number (UUID)
};
/**  @} */

#endif /* __FAST_SETUP_PROFILE__ */
