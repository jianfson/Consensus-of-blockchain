/*************************************************************************
  > File Name: SSDPUtils.h
  > Author: Jiang Xin
  > Mail: Xin.Jiang@harman.com
  > Created Time: 2018-08-17 16:35
 ************************************************************************/

#ifndef __SSDPUTILS_H__
#define __SSDPUTILS_H__

/** Global Variable **/
static struct {
    const ORA_CHAR * MSEARCH;
    const ORA_CHAR * NOTIFY;
    const ORA_CHAR * RESPONSE;

    const ORA_CHAR * HEADER_MSEARCH;
    const ORA_CHAR * HEADER_NOTIFY;
    const ORA_CHAR * HEADER_RESPONSE;

    const ORA_CHAR * ADDR_LOCALHOST;
    const ORA_CHAR * ADDR_MULTICAST;
} Global = {
    // SSDP Method
    .MSEARCH  = "M-SEARCH",
    .NOTIFY   = "NOTIFY",
    .RESPONSE = "RESPONSE",

    // SSDP Header
    .HEADER_MSEARCH  = "M-SEARCH * HTTP/1.1\r\n",
    .HEADER_NOTIFY   = "NOTIFY * HTTP/1.1\r\n",
    .HEADER_RESPONSE = "HTTP/1.1 200 OK\r\n",

    // IP Address
    .ADDR_LOCALHOST = "127.0.0.1",
    .ADDR_MULTICAST = "239.255.255.250",
};

class CSSDPUtils
{
public:
    CSSDPUtils();
    ~CSSDPUtils();

// Instance
public:
    /**
     * @brief Instance method for singlethon pattern
     *
     * @return CSSDPUtils handler.
     */
    static CSSDPUtils* GetInstance()
    {
        static CSSDPUtils s_SSDPUtils;
        return &s_SSDPUtils;
    }

public:
    string BuildSSDPSearchString();
    string GetUserId();
    string BuildSSDPAliveString();
    string BuildSSDPByebyeString();
    string ParseIP();

public:
    string ADDRESS = "239.255.255.250";
    ORA_INT32 MaxReplyTime = 5;
    
    ORA_INT32 MsgTimeout = MaxReplyTime * 1000 + 1000;

    String MSearch = "M-SEARCH * HTTP/1.1";
    String Notify = "NOTIFY * HTTP/1.1";

    String STRING_AllDevice = "ST: ssdp:all";

    String NEWLINE = "\r\n";
    String MAN = "Man:\"ssdp:discover\"";

    String LOCATION_TEXT = "LOCATION: http://";

private:
    string IP;
    ORA_INT32 port;


};

#endif//#ifndef __SSDPUTILS_H__
