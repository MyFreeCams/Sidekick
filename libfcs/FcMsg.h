#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <string>

#include "MfcJson.h"
#include "fcs.h"
#include "Log.h"
#include "fcslib_string.h"

class FcMsg : public FCMSG_Q
{
public:

    static const char* MapFcType(uint32_t dwType)
    {
        static char s_szType[64];

        switch (dwType)
        {
            case FCTYPE_NULL:               return "FCTYPE_NULL";               //  0
            case FCTYPE_LOGIN:              return "FCTYPE_LOGIN";              //  1
            case FCTYPE_ADDFRIEND:          return "FCTYPE_ADDFRIEND";          //  2
            case FCTYPE_PMESG:              return "FCTYPE_PMESG";              //  3
            case FCTYPE_STATUS:             return "FCTYPE_STATUS";             //  4
            case FCTYPE_DETAILS:            return "FCTYPE_DETAILS";            //  5
            case FCTYPE_TOKENINC:           return "FCTYPE_TOKENINC";           //  6
            case FCTYPE_ADDIGNORE:          return "FCTYPE_ADDIGNORE";          //  7
            case FCTYPE_PRIVACY:            return "FCTYPE_PRIVACY";            //  8
            case FCTYPE_ADDFRIENDREQ:       return "FCTYPE_ADDFRIENDREQ";       //  9
            case FCTYPE_USERNAMELOOKUP:     return "FCTYPE_USERNAMELOOKUP";     // 10
            case FCTYPE_ZBAN:               return "FCTYPE_ZBAN";               // 11
            case FCTYPE_BROADCASTNEWS:      return "FCTYPE_BROADCASTNEWS";      // 12
            case FCTYPE_ANNOUNCE:           return "FCTYPE_ANNOUNCE";           // 13
            case FCTYPE_MANAGELIST:         return "FCTYPE_MANAGELIST";         // 14
            case FCTYPE_INBOX:              return "FCTYPE_INBOX";              // 15
            case FCTYPE_GWCONNECT:          return "FCTYPE_GWCONNECT";          // 16
            case FCTYPE_RELOADSETTINGS:     return "FCTYPE_RELOADSETTINGS";     // 17
            case FCTYPE_HIDEUSERS:          return "FCTYPE_HIDEUSERS";          // 18
            case FCTYPE_RULEVIOLATION:      return "FCTYPE_RULEVIOLATION";      // 19
            case FCTYPE_SESSIONSTATE:       return "FCTYPE_SESSIONSTATE";       // 20
            case FCTYPE_REQUESTPVT:         return "FCTYPE_REQUESTPVT";         // 21
            case FCTYPE_ACCEPTPVT:          return "FCTYPE_ACCEPTPVT";          // 22
            case FCTYPE_REJECTPVT:          return "FCTYPE_REJECTPVT";          // 23
            case FCTYPE_ENDSESSION:         return "FCTYPE_ENDSESSION";         // 24
            case FCTYPE_TXPROFILE:          return "FCTYPE_TXPROFILE";          // 25
            case FCTYPE_STARTVOYEUR:        return "FCTYPE_STARTVOYEUR";        // 26
            case FCTYPE_SERVERREFRESH:      return "FCTYPE_SERVERREFRESH";      // 27
            case FCTYPE_SETTING:            return "FCTYPE_SETTING";            // 28
            case FCTYPE_BWSTATS:            return "FCTYPE_BWSTATS";            // 29
            case FCTYPE_TKX:                return "FCTYPE_TKX";                // 30
            case FCTYPE_SETTEXTOPT:         return "FCTYPE_SETTEXTOPT";         // 31
            case FCTYPE_SERVERCONFIG:       return "FCTYPE_SERVERCONFIG";       // 32
            case FCTYPE_MODELGROUP:         return "FCTYPE_MODELGROUP";         // 33
            case FCTYPE_REQUESTGRP:         return "FCTYPE_REQUESTGRP";         // 34
            case FCTYPE_STATUSGRP:          return "FCTYPE_STATUSGRP";          // 35
            case FCTYPE_GROUPCHAT:          return "FCTYPE_GROUPCHAT";          // 36
            case FCTYPE_CLOSEGRP:           return "FCTYPE_CLOSEGRP";           // 37
            case FCTYPE_UCR:                return "FCTYPE_UCR";                // 38
            case FCTYPE_MYUCR:              return "FCTYPE_MYUCR";              // 39
            case FCTYPE_SLAVECON:           return "FCTYPE_SLAVECON";           // 40
            case FCTYPE_SLAVECMD:           return "FCTYPE_SLAVECMD";           // 41
            case FCTYPE_SLAVEFRIEND:        return "FCTYPE_SLAVEFRIEND";        // 42
            case FCTYPE_SLAVEVSHARE:        return "FCTYPE_SLAVEVSHARE";        // 43
            case FCTYPE_ROOMDATA:           return "FCTYPE_ROOMDATA";           // 44
            case FCTYPE_NEWSITEM:           return "FCTYPE_NEWSITEM";           // 45
            case FCTYPE_GUESTCOUNT:         return "FCTYPE_GUESTCOUNT";         // 46
            case FCTYPE_PRELOGINQ:          return "FCTYPE_PRELOGINQ";          // 47
            case FCTYPE_MODELGROUPSZ:       return "FCTYPE_MODELGROUPSZ";       // 48
            case FCTYPE_ROOMHELPER:         return "FCTYPE_ROOMHELPER";         // 49
            case FCTYPE_CMESG:              return "FCTYPE_CMESG";              // 50
            case FCTYPE_JOINCHAN:           return "FCTYPE_JOINCHAN";           // 51
            case FCTYPE_CREATECHAN:         return "FCTYPE_CREATECHAN";         // 52
            case FCTYPE_INVITECHAN:         return "FCTYPE_INVITECHAN";         // 53
            case FCTYPE_RPC:                return "FCTYPE_RPC";                // 54
            case FCTYPE_QUIETCHAN:          return "FCTYPE_QUIETCHAN";          // 55
            case FCTYPE_BANCHAN:            return "FCTYPE_BANCHAN_xxx_bug";    // 56       [ deprecated ]
            case FCTYPE_PREVIEWCHAN:        return "FCTYPE_PREVIEWCHAN";        // 57
            case FCTYPE_SHUTDOWN:           return "FCTYPE_SHUTDOWN";           // 58
            case FCTYPE_LISTBANS:           return "FCTYPE_LISTBANS_xxx_bug";   // 59       [ deprecated ]         
            case FCTYPE_UNBAN:              return "FCTYPE_UNBAN_xxx_bug";      // 60       [ deprecated ]         
            case FCTYPE_SETWELCOME:         return "FCTYPE_SETWELCOME";         // 61                              
            case FCTYPE_CHANOP:             return "FCTYPE_CHANOP";             // 62                              
            case FCTYPE_LISTCHAN:           return "FCTYPE_LISTCHAN";           // 63                              
            case FCTYPE_TAGS:               return "FCTYPE_TAGS";               // 64                              
            case FCTYPE_SETPCODE:           return "FCTYPE_SETPCODE";           // 65                              
            case FCTYPE_SETMINTIP:          return "FCTYPE_SETMINTIP";          // 66                              
            case FCTYPE_UEOPT:              return "FCTYPE_UEOPT";              // 67                              
            case FCTYPE_HDVIDEO:            return "FCTYPE_HDVIDEO";            // 68                              
            case FCTYPE_METRICS:            return "FCTYPE_METRICS";            // 69                              
            case FCTYPE_OFFERCAM:           return "FCTYPE_OFFERCAM";           // 70                              
            case FCTYPE_REQUESTCAM:         return "FCTYPE_REQUESTCAM";         // 71                              
            case FCTYPE_MYWEBCAM:           return "FCTYPE_MYWEBCAM";           // 72                              
            case FCTYPE_MYCAMSTATE:         return "FCTYPE_MYCAMSTATE";         // 73                              
            case FCTYPE_PMHISTORY:          return "FCTYPE_PMHISTORY";          // 74                              
            case FCTYPE_CHATFLASH:          return "FCTYPE_CHATFLASH";          // 75                              
            case FCTYPE_TRUEPVT:            return "FCTYPE_TRUEPVT";            // 76                              
            case FCTYPE_BOOKMARKS:          return "FCTYPE_BOOKMARKS";          // 77                              
            case FCTYPE_EVENT:              return "FCTYPE_EVENT";              // 78                              
            case FCTYPE_STATEDUMP:          return "FCTYPE_STATEDUMP";          // 79                              
            case FCTYPE_RECOMMEND:          return "FCTYPE_RECOMMEND";          // 80                              
            case FCTYPE_EXTDATA:            return "FCTYPE_EXTDATA";            // 81                              
            case FCTYPE_ADMINEXT:           return "FCTYPE_ADMINEXT";           // 82                              
            case FCTYPE_SESSEVENT:          return "FCTYPE_SESSEVENT";          // 83                              
            case FCTYPE_NOTIFY:             return "FCTYPE_NOTIFY";             // 84                              
            case FCTYPE_PUBLISH:            return "FCTYPE_PUBLISH";            // 85                              
            case FCTYPE_XREQUEST:           return "FCTYPE_XREQUEST";           // 86                              
            case FCTYPE_XRESPONSE:          return "FCTYPE_XRESPONSE";          // 87                              
            case FCTYPE_EDGECON:            return "FCTYPE_EDGECON";            // 88                              
            case FCTYPE_XMESG:              return "FCTYPE_XMESG";              // 89                              
            case FCTYPE_CLUBSHOW:           return "FCTYPE_CLUBSHOW";           // 90                              
            case FCTYPE_CLUBCMD:            return "FCTYPE_CLUBCMD";            // 91                              
            case FCTYPE_AGENT:              return "FCTYPE_AGENT";              // 92                              
        //  case FCTYPE_RESERVED_93:        return "FCTYPE_RESERVED_93";        // 93       [ unused / reserved ]  
        //  case FCTYPE_RESERVED_94:        return "FCTYPE_RESERVED_94";        // 94       [ unused / reserved ]  
            case FCTYPE_ZGWINVALID:         return "FCTYPE_ZGWINVALID";         // 95                              
            case FCTYPE_CONNECTING:         return "FCTYPE_CONNECTING";         // 96                              
            case FCTYPE_CONNECTED:          return "FCTYPE_CONNECTED";          // 97                              
            case FCTYPE_DISCONNECTED:       return "FCTYPE_DISCONNECTED";       // 98                              
            case FCTYPE_LOGOUT:             return "FCTYPE_LOGOUT";             // 99                              
        }                                                                                                          
                                                                                                              
        snprintf(s_szType, sizeof(s_szType), "FCTYPE_UNKNOWN_%u", dwType);                                         
        return s_szType;
    }
                                                                                                              
            
    enum DataMode
    {
        DATA_DETECT = 0,
        DATA_JSON,
        DATA_RAW
    };

    static const size_t MAX_DATA_SZ = 1024*4096;            // 4mb as upper limit on packet size

    FcMsg()
    {
        pchMsg = NULL;
        clear();    
    }

    FcMsg(const FcMsg& copyFrom)
    {
        pchMsg = NULL;
        FCMSG msg = { copyFrom.dwMagic, copyFrom.dwType, copyFrom.dwFrom, copyFrom.dwTo, copyFrom.dwArg1, copyFrom.dwArg2, copyFrom.dwMsgLen };
        if (!buildFrom(msg, (const BYTE*)copyFrom.pchMsg))
            _MESG("FcMsg() copy ctr failed buildFrom: dwMsgLen[%u], pchMsg: 0x%X", copyFrom.dwMsgLen, copyFrom.pchMsg);
    }

    FcMsg(FCMSG msg)
    {
        pchMsg = NULL;
        if (!buildFrom(msg, NULL))
            _MESG("FcMsg() FCMSG ctr failed buildFrom: dwMsgLen[%u], pchMsg n/a", msg.dwMsgLen);
    }

    FcMsg(FCMSG_Q msg)
    {
        pchMsg = NULL;
        if (!buildFrom(msg))
            _MESG("FcMsg() FCMSG_Q ctr failed buildFrom: dwMsgLen[%u], pchMsg: 0x%X", msg.dwMsgLen, msg.pchMsg);
    }

    FcMsg(FCMSG msg, const BYTE* _pchMsg)
    {
        pchMsg = NULL;
        if (!buildFrom(msg, _pchMsg))
            _MESG("FcMsg() FCMSG,BYTE* ctr failed buildFrom: dwMsgLen[%u], pchMsg: 0x%X", msg.dwMsgLen, _pchMsg);
    }

    FcMsg(uint32_t _dwType, uint32_t _dwFrom, uint32_t _dwTo, uint32_t _dwArg1, uint32_t _dwArg2, uint32_t _dwMsgLen, const BYTE* _pchMsg)
    {
        pchMsg = NULL;
        FCMSG_Q msg = { FCPROTOCOL_MAGIC, _dwType, _dwFrom, _dwTo, _dwArg1, _dwArg2, _dwMsgLen, (char*)_pchMsg };
        if (!buildFrom(msg))
            _MESG("FcMsg() list ctr failed buildFrom: dwMsgLen[%u], pchMsg: 0x%X", msg.dwMsgLen, _pchMsg);
    }

    FcMsg(uint32_t _dwType, uint32_t _dwFrom, uint32_t _dwTo, uint32_t _dwArg1, uint32_t _dwArg2, const MfcJsonObj& js)
    {
        dwMagic  = FCPROTOCOL_MAGIC;
        dwType   = _dwType;
        dwFrom   = _dwFrom;
        dwTo     = _dwTo;
        dwArg1   = _dwArg1;
        dwArg2   = _dwArg2;
        dwMsgLen = 0;
        pchMsg   = NULL;
 
        string sMsg;
        if (js.Serialize(sMsg) > 0)
        {
            if (sMsg.size() < MAX_DATA_SZ)
            {
                dwMsgLen = (uint32_t)sMsg.size();
                pchMsg = (char*)calloc(1, dwMsgLen+1);
                assert(pchMsg);
                memcpy(pchMsg, sMsg.c_str(), dwMsgLen);
            }
            else _MESG("Can't allocate msgdata length[%u] >= MAX_DATA_SZ[%u]", sMsg.size(), MAX_DATA_SZ);
        }
    }

    FcMsg(uint32_t _dwType, uint32_t _dwFrom, uint32_t _dwTo, uint32_t _dwArg1, uint32_t _dwArg2, const string& sMsg)
    {
        dwMagic  = FCPROTOCOL_MAGIC;
        dwType   = _dwType;
        dwFrom   = _dwFrom;
        dwTo     = _dwTo;
        dwArg1   = _dwArg1;
        dwArg2   = _dwArg2;
        pchMsg   = NULL;
        dwMsgLen = 0;
       
        if (sMsg.size() > 0)
        {
            if (sMsg.size() < MAX_DATA_SZ)
            {
                dwMsgLen = (uint32_t)sMsg.size();
                pchMsg = (char*)calloc(1, dwMsgLen+1);
                assert(pchMsg);
                memcpy(pchMsg, sMsg.c_str(), dwMsgLen);
            }
            else _MESG("Can't allocate msgdata length[%u] >= MAX_DATA_SZ[%u]", sMsg.size(), MAX_DATA_SZ);
        }
    }

    ~FcMsg()
    {
        clear();
    }

    bool buildFrom(FCMSG msg, const BYTE* _pchMsg = NULL)
    {
        bool retVal = true;

        clear();

        if (msg.dwMagic == FCPROTOCOL_MAGIC_NETORDER)
        {
            dwMagic  = ntohl(msg.dwMagic);
            dwType   = ntohl(msg.dwType);
            dwFrom   = ntohl(msg.dwFrom);
            dwTo     = ntohl(msg.dwTo);
            dwArg1   = ntohl(msg.dwArg1);
            dwArg2   = ntohl(msg.dwArg2);
            dwMsgLen = ntohl(msg.dwMsgLen);
        }
        else
        {
            // dwMagic is sometimes overridden when the msg is embedded in another, so don't
            // require it to be FCPROTOCOL_MAGIC
            dwMagic  = msg.dwMagic;
            dwType   = msg.dwType;
            dwFrom   = msg.dwFrom;
            dwTo     = msg.dwTo;
            dwArg1   = msg.dwArg1;
            dwArg2   = msg.dwArg2;
            dwMsgLen = msg.dwMsgLen;
        }

        if (dwMsgLen > 0 && dwMsgLen < MAX_DATA_SZ-1 && _pchMsg != NULL)
        {
            pchMsg = (char*)calloc(1, dwMsgLen+1);
            assert(pchMsg);
            memcpy(pchMsg, _pchMsg, dwMsgLen);
        }
        else
        {
            if (dwMsgLen > 0)
            {
                _MESG("Can't allocate msgdata, no ptr[0x%X] to data or size[%u] too big", _pchMsg, dwMsgLen);
                retVal = false;
            }
            dwMsgLen = 0;
            pchMsg = NULL;
        }

        return retVal;
    }

    bool buildFrom(FCMSG_Q msg)
    {
        FCMSG msg2 = { msg.dwMagic, msg.dwType, msg.dwFrom, msg.dwTo, msg.dwArg1, msg.dwArg2, msg.dwMsgLen };
        return buildFrom(msg2, (const BYTE*)msg.pchMsg);
    }

    void clear(void)
    {
        if (pchMsg)
            free(pchMsg);

        dwMagic  = 0;
        dwType   = 0;
        dwFrom   = 0;
        dwTo     = 0;
        dwArg1   = 0;
        dwArg2   = 0;
        dwMsgLen = 0;
        pchMsg   = NULL;
    }

    bool js(MfcJsonObj& j, DataMode dataMode = DATA_DETECT)
    {
        DataMode mode = DATA_RAW;

        j.clear();
        j.objectAdd("type", dwType);
        j.objectAdd("from", dwFrom);
        j.objectAdd("to",   dwTo);
        j.objectAdd("arg1", dwArg1);
        j.objectAdd("arg2", dwArg2);

        if (pchMsg == NULL)
        {
            // If no message payload is passed in, embed dwMsgLen blind (it may still describe the expected data
            // length in cases such as FCTYPE_EXTDATA where the length is needed and no data itself is embedded)
            j.objectAdd("len", dwMsgLen);
        }
        else if (dwMsgLen > 0)
        {
            // If message payload IS provided, only process if dwMsgLen is also > 0, depending on the data type
            // or datamode, dwMsgLen may be re-calculated after message payload processing, or not be included
            // at all in DATA_JSON mode.

            mode = DATA_RAW;                                    // Default to raw data
            
            if (dataMode == DATA_DETECT)                        // If in detect mode, check for array or object start
            {
                if (pchMsg[0] == '[' || pchMsg[0] == '{')       // Change to DATA_JSON if it looks like its an object or array
                    mode = DATA_JSON;
            }

            string sData((const char*)pchMsg, (size_t)dwMsgLen);
                
            if (mode == DATA_JSON)
            {
                MfcJsonPtr pObj = MfcJsonObj::newType(JSON_T_OBJECT);
                if ( ! pObj->Deserialize(sData))
                {
                    mode = DATA_RAW;                // Fall back to raw mode if deserialize fails
                    delete pObj;
                    pObj = NULL;
                }
                else j.objectAdd("data", pObj);     // Add data in json mode
            }

            // Either originally set to raw, or falling back to raw if json deserialize failed
            if (mode == DATA_RAW)
            {
                j.objectAdd("len", (uint64_t)sData.size());
                j.objectAdd("data", sData);
            }
        }

        return true;
    }

    MfcJsonObj js(void)
    {   
        MfcJsonObj j;
        js(j);
        return j;
    }

    const char* payload_str(MfcJsonObj& js)
    {
        return (js.m_dwType != JSON_T_NONE ? js.prettySerialize().c_str() : (dwMsgLen > 0 ? pchMsg : ""));
    }

    // Helper for returning string of serialized json object
    string jstr(void)
    {
        return js().Serialize();
    }

    const char* cstr(void)
    {
        return js().Serialize().c_str();
    }

    bool Deserialize(const MfcJsonObj& j)
    {
        bool fRet = false;
        MfcJsonObj* pVal;   
        int64_t nVal;

        clear();

        // get g.type, from, to, arg1, arg2, data, and len ....
        if (j.objectGetInt("type", nVal))
        {
            dwType = (uint32_t)nVal;

            if (j.objectGetInt("from", nVal))
            {
                dwFrom = (uint32_t)nVal;

                if (j.objectGetInt("to", nVal))
                {
                    dwTo = (uint32_t)nVal;

                    if (j.objectGetInt("arg1", nVal))
                    {
                        dwArg1 = (uint32_t)nVal;

                        if (j.objectGetInt("arg2", nVal))
                        {
                            dwArg2 = (uint32_t)nVal;

                            string sData;

                            // string sData = string((const char*)pchMsg, (size_t)dwMsgLen);
                            if (j.objectGetInt("len", nVal))
                            {
                                dwMsgLen = (uint32_t)nVal;
                                j.objectGetString("data", sData);
                            }
                            else if (j.objectGetObject("data", &pVal) && pVal)
                            {
                                sData = pVal->Serialize();
                                dwMsgLen = (uint32_t)sData.size();
                                pVal = NULL;
                            }

                            if (dwMsgLen > 0 && dwMsgLen == (uint32_t)sData.size() && dwMsgLen < MAX_DATA_SZ-1)
                            {
                                if (pchMsg)
                                    free(pchMsg);
                                pchMsg = (char*)calloc(1, dwMsgLen+1);
                                assert(pchMsg);
                                memcpy(pchMsg, sData.c_str(), dwMsgLen);
                            }

                            if (dwMsgLen != (uint32_t)sData.size())
                                clear();    
                            else
                                fRet = true;
                        }
                    }
                }
            }
        }

        if (fRet)
            dwMagic = FCPROTOCOL_MAGIC;

        return fRet;
    }

    FCMSG Msg(char** ppchMsg = NULL)
    {
        FCMSG msg = { dwMagic, dwType, dwFrom, dwTo, dwArg1, dwArg2, dwMsgLen };
        
        if (ppchMsg)
            *ppchMsg = pchMsg;

        return msg;
    }

    FCMSG_Q MsgQ(void)
    {
        FCMSG_Q msg = { dwMagic, dwType, dwFrom, dwTo, dwArg1, dwArg2, dwMsgLen, pchMsg };
        return msg;
    }


    //
    // convert FCMSG properties to text format for sending to websocket clients.
    // Writes out 6 digit length and a space character for the decimal int length
    // formatting used by websock text protocol. Writes output message format to
    // sOut argument.
    //
    static size_t writeToWebsock(   string&     sOut,
                                    bool        encodePayload,
                                    uint32_t    dwType,
                                    uint32_t    dwFrom,
                                    uint32_t    dwTo,
                                    uint32_t    dwArg1,
                                    uint32_t    dwArg2,
                                    uint32_t    dwMsgLen,
                                    const char* pchMsg      )
                                    
    {
        static char szEncData[FCMAX_CLIENTPACKET * 4], szLen[32];
        BYTE* pchMsgData        = (BYTE*)pchMsg;
        const char* pszFmt      = "%06d%u %u %u %u %u %s";
        size_t nLenDigits       = 6;

        if (dwMsgLen > 0 && pchMsgData)
        {
            if (encodePayload)
            {
                // Write string data out with a light-weight version of URI encoding, catching only the
                // characters which would break XMLSockets for flash clients
                if (dwMsgLen < sizeof(szEncData) / 4)
                {
                    size_t nDx = 0;
                    for (size_t nCx = 0; nCx < (size_t)dwMsgLen && nDx < sizeof(szEncData) - 8; nCx++)
                    {
                        if (MfcJsonObj::encodeChar( ((unsigned char)pchMsgData[nCx]) ))
                        {
                            szEncData[nDx+0] = '%';
                            szEncData[nDx+1] = MfcJsonObj::sm_pszHexVals[((unsigned char)pchMsgData[nCx]) / 16];
                            szEncData[nDx+2] = MfcJsonObj::sm_pszHexVals[((unsigned char)pchMsgData[nCx]) % 16];
                            nDx+=3;
                        }
                        else szEncData[nDx++] = pchMsgData[nCx];
                    }
                    szEncData[nDx] = '\0';

                    // Write formatted and encoded payload to string with 0 as initial length param
                    // (which we'll rewrite once the real length is calculated)
                    stdprintf(sOut, pszFmt, 0, dwType, dwFrom, dwTo, dwArg1, dwArg2, szEncData);
                }
            }
            else stdprintf(sOut, pszFmt, 0, dwType, dwFrom, dwTo, dwArg1, dwArg2, pchMsgData);
        }
        // No message data, encode just integers (uses empty string for last arg)
        else stdprintf(sOut, pszFmt, 0, dwType, dwFrom, dwTo, dwArg1, dwArg2, "");

        // Calculate the length of the string without the 6 digiit frame length prefix,
        // then write that length out to the first 6 digits of sOut.c_str()
        size_t nMsgSz = sOut.size() - nLenDigits;
        snprintf(szLen, sizeof(szLen), "%06d", (int)nMsgSz);
        memcpy((void*)sOut.c_str(), szLen, nLenDigits);

        return nMsgSz; 
    }

    //
    // convert FCMSG properties to text format for sending to websocket clients
    // (without writing length prefixes, this method assumes websocket library will
    // handle that)
    //
    static size_t textMsg(  string&     sOut,
                            bool        encodePayload,
                            uint32_t    dwType,
                            uint32_t    dwFrom,
                            uint32_t    dwTo,
                            uint32_t    dwArg1,
                            uint32_t    dwArg2,
                            uint32_t    dwMsgLen,
                            const char* pchMsg   )
    {
        static char szEncData[FCMAX_CLIENTPACKET * 4];
        BYTE* pchMsgData        = (BYTE*)pchMsg;
        const char* pszFmt      = "%u %u %u %u %u %s";

        if (dwMsgLen > 0 && pchMsgData)
        {
            // Write string data out with a light-weight version of URI encoding, catching only the
            // characters which would break XMLSockets for flash clients
            if (encodePayload)
            {
                if (dwMsgLen < sizeof(szEncData) / 4)
                {
                    size_t nDx = 0;
                    for (size_t nCx = 0; nCx < (size_t)dwMsgLen && nDx < sizeof(szEncData) - 8; nCx++)
                    {
                        if (MfcJsonObj::encodeChar( ((unsigned char)pchMsgData[nCx]) ))
                        {
                            szEncData[nDx+0] = '%';
                            szEncData[nDx+1] = MfcJsonObj::sm_pszHexVals[((unsigned char)pchMsgData[nCx]) / 16];
                            szEncData[nDx+2] = MfcJsonObj::sm_pszHexVals[((unsigned char)pchMsgData[nCx]) % 16];
                            nDx+=3;
                        }
                        else szEncData[nDx++] = pchMsgData[nCx];
                    }
                    szEncData[nDx] = '\0';

                    // Write formatted and encoded payload to string 
                    stdprintf(sOut, pszFmt, dwType, dwFrom, dwTo, dwArg1, dwArg2, szEncData);
                }
            }
            else stdprintf(sOut, pszFmt, dwType, dwFrom, dwTo, dwArg1, dwArg2, pchMsgData);
        }
        // No message data, encode just integers (uses empty string for last arg)
        else stdprintf(sOut, pszFmt, dwType, dwFrom, dwTo, dwArg1, dwArg2, "");
        
        return sOut.size(); 
    }

    // helpers for primary implementation of writeToText()
    static size_t textMsg(string& sOut, bool encodePayload, uint32_t dwType, uint32_t dwFrom, uint32_t dwTo, uint32_t dwArg1, uint32_t dwArg2, const string& sData)    
    {
        return textMsg(sOut, encodePayload, dwType, dwFrom, dwTo, dwArg1, dwArg2, (uint32_t)sData.size(), sData.c_str());
    }

    static size_t textMsg(string& sOut, bool encodePayload, uint32_t dwType, uint32_t dwFrom, uint32_t dwTo, uint32_t dwArg1, uint32_t dwArg2, MfcJsonObj& jsData)
    {
        string sData;
        jsData.Serialize(sData);
        return textMsg(sOut, encodePayload, dwType, dwFrom, dwTo, dwArg1, dwArg2, (uint32_t)sData.size(), sData.c_str());
    }

    static string textMsg(bool encodePayload, uint32_t dwType, uint32_t dwFrom, uint32_t dwTo, uint32_t dwArg1, uint32_t dwArg2, uint32_t dwMsgLen, const char* pchData)
    {
        string sMsg;
        textMsg(sMsg, encodePayload, dwType, dwFrom, dwTo, dwArg1, dwArg2, dwMsgLen, pchData);
        return sMsg;
    }

    static string textMsg(bool encodePayload, uint32_t dwType, uint32_t dwFrom, uint32_t dwTo, uint32_t dwArg1, uint32_t dwArg2, const string& sData)
    {
        string sMsg;
        textMsg(sMsg, encodePayload, dwType, dwFrom, dwTo, dwArg1, dwArg2, sData);
        return sMsg;
    }

    static string textMsg(bool encodePayload, uint32_t dwType, uint32_t dwFrom, uint32_t dwTo, uint32_t dwArg1, uint32_t dwArg2, MfcJsonObj& jsData)
    {
        string sMsg;
        textMsg(sMsg, encodePayload, dwType, dwFrom, dwTo, dwArg1, dwArg2, jsData);
        return sMsg;
    }

    static string textMsg(bool encodePayload, FcMsg& msg)
    {
        string sMsg;
        textMsg(sMsg, encodePayload, msg.dwType, msg.dwFrom, msg.dwTo, msg.dwArg1 , msg.dwArg2, msg.dwMsgLen, msg.pchMsg);
        return sMsg;
    }

    // read text format message into FCMSG/FCMSG_Q from websocket, ajax, or flashsocket clients.
    // returns true if successfully built this object from sMsg text. If the payload starts with
    // a '%7b' or a '%5b', it is assumed to be URI encoded, and the data is decoded with
    // MfcJsonObj::decodeURIComponent().  If the data then appears to be a json object (starting
    // with a '{' or '['), then the data will be deserialized into pJsData if the pJsData argument
    // points to a MfcJsonObj.
    //
    // If partialBuf is not empty, uses its contents first before appending sMsg data
    // when parsing. Updates partialBuf to be remainder of sMsg (if sMsg cross frame boundaries),
    // or resets partialBuf to empty if partialBuf + sMsg resolve to all completed frames.
    // 
    bool readFromText(string& partialBuf, string& sMsg, MfcJsonPtr pJsData = NULL)
    {
        uint32_t dwFrameLen = 0;
        bool retVal = false;
        strVec vArgs;

        if (!partialBuf.empty())
        {
            sMsg.insert(0, partialBuf);
            partialBuf.clear();
            //_MESG("readFromText now using sMsg sz of %zu:  %s", sMsg.size(), sMsg.c_str());
        }

        clear();

        // assume sMsg is the start of a msg, should always have 6 digit number first
        if (sMsg.size() > 6)
        {
            dwFrameLen = atoi(sMsg.substr(0,6).c_str());
            sMsg.erase(0, 6);
        }

        if (sMsg.size() > dwFrameLen)
        {
            partialBuf = sMsg.substr(dwFrameLen);
            sMsg.erase(dwFrameLen);
        }

        if (stdsplit(sMsg, ' ', vArgs) >= 5)
        {
            dwMagic = FCPROTOCOL_MAGIC;

            /*
            // If the first argument has more than 6 characters and is all digits, then
            // it would appear ther websocket frame length prefix wasn't stripped from
            // the message, and we'll want to split vArgs[0] into the frame length and dwType
            if (vArgs[0].size() > 6 && std::all_of(vArgs[0].begin(), vArgs[0].end(), ::isdigit))
            {
                string sLen = vArgs[0].substr(0, 6);
                string sType = vArgs[0].substr(6);
                dwType = atoi(sType.c_str());
                dwFrameLen = atoi(sLen.c_str());
            }
            else*/

            dwType  = atoi(vArgs[0].c_str());

            dwFrom  = atoi(vArgs[1].c_str());
            dwTo    = atoi(vArgs[2].c_str());
            dwArg1  = atoi(vArgs[3].c_str());
            dwArg2  = atoi(vArgs[4].c_str());
            dwMsgLen = 0;
            string sData;
            pchMsg = NULL;            

            
            // Copy optional payload, if found
            if (vArgs.size() > 5)
            {
                // Build remainder of string from [5]... as sData
                for (size_t n = 5; n < vArgs.size(); n++)
                {
                    if (n > 5) sData += " ";
                    sData += vArgs[n];
                }

                if (sData.size() > 0)
                {
                    size_t nSz = sData.size() + 1;

                    if (nSz < MAX_DATA_SZ)
                    {
                        // Skip '-' as a payload, in text format this was used as an empty payload
                        if (sData != "-" && nSz < MAX_DATA_SZ)
                        {
                            //
                            // if sData starts with '%7b'or '%5b' (uri encoded '{' and '['), then
                            // assume the payload is a URI encoded json object or array, and decode
                            // and deserialize appropriately
                            //
                            if (    ( sData.size() > 3                          )
                                &&  ( sData.at(0) == '%'                        )
                                &&  ( sData.at(1) == '7' || sData.at(1) == '5'  )
                                &&  ( tolower(sData.at(2)) == 'b'               )   )
                            {
                                MfcJsonObj::decodeURIComponent(sData);
                            }

                            if (pJsData)
                            {
                                if (sData.at(0) == '{' || sData.at(0) == '[')
                                {
                                    pJsData->Deserialize(sData);
                                }
                            }

                            // Account for zero terminator in allocation, but not in dwMsgLen
                            dwMsgLen = (uint32_t)sData.size();

                            if (pchMsg)
                                free(pchMsg);
                            pchMsg = (char*)calloc(1, dwMsgLen + 1);
                            assert(pchMsg);

                            fcs_strlcpy(pchMsg, sData.c_str(), dwMsgLen + 1);
                        }
                    }
                    else _MESG("Can't allocate msgdata length[%u] too big", nSz);
                }
            }

            if (sMsg.size() == dwFrameLen)
            {
                //_MESG("[DBG Edge] sMsg sz:%zu == dwFrameLen:%u  + 6, as expected; dwMsgLen: %u", sMsg.size(), dwFrameLen, dwMsgLen);
            }
            else if (sMsg.size() > dwFrameLen)
            {
                sMsg.erase(0, dwFrameLen);
                _MESG("[ERR Edge] sMsg larger than 1 frame, moving remainder %zu to partialBuf: %s", sMsg.size(), sMsg.c_str());
                partialBuf.insert(0, sMsg);
            }
            else
            {
                // sMsg is a paretial frame, move entirely to partialBuf
                _MESG("[ERR Edge] sMsg less than 1 frame, moving all %zu to partialBuf: %s", sMsg.size(), sMsg.c_str());
                partialBuf.insert(0, sMsg);
            }
            //_MESG("[ERR Edge] sMsg sz:%zu != dwFrameLen:%u  + 6; dwMsgLen: %u", sMsg.size(), dwFrameLen, dwMsgLen);

            retVal = true;
        }
        else _MESG("[ERR Edge] Unable to parse text msg \"%s\" into 5 or more space separated elements", sMsg.c_str());

        return retVal;
    }

};

