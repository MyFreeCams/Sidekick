#pragma once

//	APPDATA_SUB is the directory used off of CSIDL_LOCAL_APPDATA to store main program & files in, 
// for example "C:\Users\<username>\AppData\Local\MyFreeCams if APPDATA_SUB was just "MyFreeCams"
#define APPDATA_SUB							"MyFreeCams"

#define XCHAT__SESSION_EV_FMT				"__FCClient::NetThreadEvHandle(%d)__"
#define FRIENDCONNECTOR_MAIN_CLASSWND	    "__FCClient::FriendConnectorClientClass__"

//------------------------------------------------------------------------
// Client version - posted to server in FCTYPE_LOGIN
//
#define CLIENT_VERSION		220181104
//------------------------------------------------------------------------

#pragma warning( disable: 4838 )
#pragma warning( disable: 4091 )

// production build?
#define _PRODUCT_MODE				1

// dev build? 
//#  define _DEVELOP_MODE				1

// Auto-detect if we're in dev mode or production mode
// this can be overridden by setting one of the _SERVERS
// defines up above though.
#ifdef  _DEVELOP_MODE
#ifndef _DEVELOP_SERVERS
#ifndef _PRODUCT_SERVERS
#define _DEVELOP_SERVERS		1
#endif
#endif
#elif   _PRODUCT_MODE
#ifndef _DEVELOP_SERVERS
#ifndef _PRODUCT_SERVERS
#define _PRODUCT_SERVERS		1
#endif
#endif
#endif