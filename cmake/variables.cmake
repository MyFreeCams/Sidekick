function(SET_VARIABLES)
	if(DEFINED ENV{MFC_AGENT_SVC_URL})
		set(MFC_AGENT_SVC_URL "$ENV{MFC_AGENT_SVC_URL}" CACHE STRING "URL to poll for OAuth response data")
	else()
		set(MFC_AGENT_SVC_URL "https://sidekick.mfc.dev/agentSvc.php" CACHE STRING "URL to poll for OAuth response data")
	endif()

	set(MFC_LOG_LEVEL "ILog::LogLevel::MAX_LOGLEVEL" CACHE STRING "Default logging level")
	set(MFC_LOG_OUTPUT_MASK "10" CACHE STRING "Default logging output mask")
	set(MFC_NO_UPDATES "0" CACHE STRING "Flag to turn off the updates. ObsUpdater will do everything but update")
	set(MFC_DO_CEF_BUILD "1" CACHE STRING "Flag to turn off CEF builds. Set to FALSE for no CEF build")
	set(MFC_NO_AUTOSTART_CEFLOGIN "0" CACHE STRING "Set to TRUE to disable MFCCefLogin auto start")
	set(MFC_BROWSER_LOGIN "0" CACHE STRING "Flag to enable browser panel for MFC login (instead of CEF Login App)")
	set(MFC_AGENT_EDGESOCK "1" CACHE STRING "Flag to enable websocket agent")

	if(MFC_BROWSER_LOGIN)
		set(MFC_BROWSER_AVAILABLE "1" CACHE STRING "Flag to enable building MFC customized browser panel. Set MFC_BROWSER_LOGIN=1 to use browser panel for login")
	endif()

	set(MFC_CEF_LOGIN_URL "https://sidekick.mfc.dev/?_x=login&_t=q&_cv=plugin" CACHE STRING "URL for login page")
	set(MFC_CEF_API_OBJECT "fcsAPI" CACHE STRING "Name of the javascript object added to the web page")
	set(MFC_CEF_API_DOLOGIN "doLogin" CACHE STRING "Name of the doLogin function added to the fcsAPI object")
	set(MFC_CEF_API_MODEL_STREAM_KEY "SetMTX" CACHE STRING "Name of the setMTX function added to the fcsAPI object")

	if(APPLE)
		set(MFC_OBS_PLUGIN_ROOT_PATH "/Library/Application Support/obs-studio" CACHE PATH "Install root for plugins")
		set(MFC_OBS_PLUGIN_BIN_PATH "${MFC_OBS_PLUGIN_ROOT_PATH}/MFCBroadcast/bin" CACHE PATH "Install bin path for plugins")
		set(MFC_OBS_PLUGIN_DATA_PATH "${MFC_OBS_PLUGIN_ROOT_PATH}/MFCBroadcast/data" CACHE PATH "Install data path for plugins")
		set(MFC_OBS_PLUGIN_LOG_PATH "${MFC_OBS_PLUGIN_ROOT_PATH}/MFCBroadcast/Logs" CACHE PATH "Path to the log files (no '/' at the end)")

		set(MFC_OBS_PLUGIN_ROOT_PATH_BUILD "${CMAKE_BINARY_DIR}/MFCBroadcast" CACHE PATH "Build root for MFCBroadcast plugin")
		set(MFC_OBS_PLUGIN_BIN_PATH_BUILD "${MFC_OBS_PLUGIN_ROOT_PATH_BUILD}/bin" CACHE PATH "Build bin path for MFCBroadcast plugin")

		set(MFC_OBS_CEF_LOGIN_BIN_PATH "${MFC_OBS_PLUGIN_ROOT_PATH}/sidekick" CACHE PATH "Install binary path for MFCCefLogin app")
		set(MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD "${MFC_OBS_PLUGIN_ROOT_PATH_BUILD}/MFCCefLogin" CACHE PATH "Build binary path for MFCCefLogin app")
		set(MFC_CEF_APP_EXE_NAME "MFCCefLogin.app" CACHE STRING "Name of cef login app")
	else()
		set(PUB_ENV "PUBLIC")
		set(MFC_OBS_PLUGIN_ROOT_PATH "$ENV{${PUB_ENV}}/Sidekick" CACHE PATH "Install root for plugins")
		set(MFC_OBS_PLUGIN_BIN_PATH "${MFC_OBS_PLUGIN_ROOT_PATH}/bin" CACHE PATH "Install bin path for plugins")
		set(MFC_OBS_PLUGIN_DATA_PATH "${MFC_OBS_PLUGIN_ROOT_PATH}/data" CACHE PATH "Install data path for plugins")
		set(MFC_OBS_PLUGIN_LOG_PATH "${MFC_OBS_PLUGIN_ROOT_PATH}/Log" CACHE PATH "Path to the log files (no '/' at the end)")

		set(MFC_OBS_PLUGIN_ROOT_PATH_BUILD "${CMAKE_BINARY_DIR}/Sidekick" CACHE PATH "Build root for plugins")
		set(MFC_OBS_PLUGIN_BIN_PATH_BUILD "${MFC_OBS_PLUGIN_ROOT_PATH_BUILD}/bin" CACHE PATH "Build bin path for plugins")

		set(MFC_OBS_CEF_LOGIN_BIN_PATH "${MFC_OBS_PLUGIN_ROOT_PATH}/cef" CACHE PATH "Install binary path for MFCCefLogin app")
		set(MFC_OBS_CEF_LOGIN_BIN_PATH_BUILD "${MFC_OBS_PLUGIN_ROOT_PATH_BUILD}/cef" CACHE PATH "Build binary path for MFCCefLogin app")
		set(MFC_CEF_APP_EXE_NAME "MFCCefLogin.exe" CACHE STRING "Name of cef login app")

		set(PF_ENV "PROGRAMFILES")
		set(OBS_APP_INST_PATH "$ENV{${PF_ENV}}/obs-studio" CACHE PATH "Location of OBS installed on system")
	endif()

	# used in the post-build copy command for MFCUpdater (Windows)
	set(OBS_RUNDIR_PLUGIN_PATH "${OBS_OUTPUT_DIR}/$<CONFIG>/${OBS_PLUGIN_DESTINATION}" CACHE PATH "OBS rundir plugin path")

	if(BROWSER_PANEL)
		set(MFC_BROWSER_AVAILABLE "1" CACHE STRING "Flag to enable building MFC customized browser panel. Set MFC_BROWSER_LOGIN=1 to use browser panel for login")
		set(BROWSER_AVAILABLE "1")
	else()
		if(DEFINED ENV{BROWSER_PANEL})
			set(MFC_BROWSER_AVAILABLE $ENV{BROWSER_PANEL} CACHE STRING "Flag to enable building MFC customized browser panel. Set MFC_BROWSER_LOGIN=1 to use browser panel for login")
		else()
			set(MFC_BROWSER_AVAILABLE "0" CACHE STRING "Flag to enable building MFC customized browser panel. Set MFC_BROWSER_LOGIN=1 to use browser panel for login")
		endif()
	endif()
endfunction()
