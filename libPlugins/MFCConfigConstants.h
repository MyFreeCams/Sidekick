/*
 * Copyright (c) 2013-2020 MFCXY, Inc. <mfcxy@mfcxy.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#ifndef MFC_CONFIG_CONSTANTS_H_
#define MFC_CONFIG_CONSTANTS_H_


#ifdef _WIN64
#define BIT_STRING                              "64bit"
#elif _WIN32
#define BIT_STRING                              "32bit"
#endif

#define PLATFORM_WIN64                          "Win64"
#define PLATFORM_WIN32                          "Win32"
#define PLATFORM_MAC                            "Mac"

// OBS profile variables.
#define CONFIG_SECTION                          "MFC"
#define CONFIG_URL                              "url"
#define CONFIG_KEY                              "KEY"
#define CONFIG_MODELID                          "modelid"
#define CONFIG_HEARTBEAT_INTERVAL               "HRTBEAT"
#define CONFIG_DEADCNT                          "DEADCNT"
#define CONFIG_SEND_LOGS                        "SendLogs"
#define CONFIG_UPD_UPDATER                      "UpdUpd"
#define CONFIG_UPD_SYSREP                       "UpdSR"
#define CONFIG_VERSION                          "Version"
#define CONFIG_ALLOW_CONNECT                    "AllowConnect"
#define CONFIG_CAM_SCORE                        "camscore"
#define CONFIG_SESSION_ID                       "sid"
#define CONFIG_PROTOCOL                         "protocol"
#define CONFIG_HEIGHT                           "height"
#define CONFIG_WIDTH                            "width"
#define CONFIG_CODEC                            "codec"
#define CONFIG_FRAME_RATE                       "framerate"
#define CONFIG_MODEL_USER_NAME                  "modeluser"
#define CONFIG_MODEL_PWD                        "pwd"
#define CONFIG_TOKEN                            "sidekick_tok"
#define CONFIG_TOKENSTAMP                       "sidekick_tok_tm"

// REST API interface defaults.
#ifdef _PROD
#ifndef MFC_API_SVR
#define MFC_API_SVR                             "https://api-a.myfreecams.com/fcsagent"
#endif
#ifndef MFC_FILE_SVR
#define MFC_FILE_SVR                            "https://api-a.myfreecams.com/modelagent"
#endif
#else // _PROD
#ifndef MFC_API_SVR
#define MFC_API_SVR                             "https://zd7-a.myfreecams.com/api/fcsagent"
#endif
#ifndef MFC_FILE_SVR
#define MFC_FILE_SVR                            "https://zd7-a.myfreecams.com/modelagent"
#endif
#endif // _PROD

#ifndef MFC_AGENT_SVC_URL
#define MFC_AGENT_SVC_URL                       "https://sidekick.mfc.dev/yagentSvc.php"
#endif

#define SYSREPORT_API                           "/SysInfo"
#define STARTUP_API                             "/Startup"
#define SHUTDOWN_API                            "/Shutdown"
#define MODELVERSION_API                        "/ModelVersion"
#define HEARTBEAT_API                           "/Heartbeat"

#ifndef DEFAULT_HEARTBEAT_INTERVAL
#define DEFAULT_HEARTBEAT_INTERVAL              60
#endif

#ifndef DEFAULT_DEAD_COUNT
#define DEFAULT_DEAD_COUNT                      5
#endif

#ifndef MFC_MAX_DEADCOUNT
#define MFC_MAX_DEADCOUNT                       10
#endif

#ifndef MFC_MAX_SLEEP_INTERVAL
#define MFC_MAX_SLEEP_INTERVAL                  600
#endif

#ifndef DEFAULT_SESSION_TICKET
#define DEFAULT_SESSION_TICKET                  ""
#endif

#ifndef DEFAULT_VERSION
#define DEFAULT_VERSION                         "default"
#endif

#define MFC_DEFAULT_WEBRTC_OUTPUT               "mfc_wowza_output"
#define MFC_DEFAULT_WEBRTC_PROTOCOL             "TCP"
#define MFC_DEFAULT_WEBRTC_CODEC                "h264"
#define MFC_DEFAULT_WEBRTC_WIDTH                1280
#define MFC_DEFAULT_WEBRTC_HEIGHT               720
#define MFC_DEFAULT_WEBRTC_FRAMERATE            30

#define JSON_MODEL_STRM_KEY                     "modelStreamingKey"
#define JSON_PLUGIN_TYPE                        "pluginType"
#define JSON_MODEL_USR_ID                       "modelUserID"
#define JSON_CTX                                "sessionCtx"
#define JSON_TKX                                "sessionTkx"
#define JSON_MODEL_USER                         "modelUserName"
#define JSON_MODEL_PWD                          "modelPassword"

// Plugin names
#define PLUGIN_TYPE_UPDATER                     1
#define PLUGIN_TYPE_BROADCAST                   2

#define UPDATER_FILENAME                        "MFCUpdater"
#define BROADCAST_FILENAME                      "MFCBroadcast"

// REST API errors
#define ERR_BAD_PARAMETER                       22
#define ERR_UNK                                 1500
#define ERR_AUTH_DENY                           13
#define ERR_METHOD_NOT_FND                      2
#define ERR_NO_RESPONSE                         -1
#define ERR_FILE_ERROR                          -2
#define ERR_NEED_LOGIN                          -3

// REST API JSON members (response)
#define STARTUP_SESSION_TICKET                  "s_ctx"
#define STARTUP_SESSION_TKX                     "s_tkx"
#define STARTUP_HEARTBEAT_INTERVAL              "HeartbeatInterval"
#define STARTUP_BROADCAST_URL                   "url"
#define STARTUP_DEADCNT                         "deadCount"
#define STARTUP_SENDLOGS                        "sendlogs"
#define STARTUP_UPD_UPDATER                     "updupd"
#define STARTUP_UPD_SYSREPORT                   "updsysreport"
#define STARTUP_ALLOWCONNECTION                 "allowConnection"

#define STARTUP_MODEL_SESSION_ID                "sid"
#define STARTUP_MODEL_CAMSCORE                  "camscore"
#define STARTUP_WEBRTC_WIDTH                    "width"
#define STARTUP_WEBRTC_HEIGHT                   "height"
#define STARTUP_WEBRTC_PROTOCOL                 "protocol"
#define STARTUP_WEBRTC_CODEC                    "codec"
#define STARTUP_WEBRTC_USER                     "model"
#define STARTUP_WEBRTC_PWD                      "ctx"
#define STARTUP_WEBRTC_MID                      "modelid"

// REST API retry interval
#define ERROR_SLEEP_TIMER                       5

#ifndef STREAMING_SERVICE_NAME
#define STREAMING_SERVICE_NAME                  "MyFreeCams Streaming Server"
#endif

#define OBS_SETTINGS_URL                        "URL"
#define OBS_SETTINGS_KEY                        "KEY"
#define OBS_SETTINGS_USERNAME                   "username"

#define MANIFEST_FILE                           "manifest.txt"

#define HTTP_RESP_ERR                           "err"
#define HTTP_RESP_ERRMSG                        "errmsg"
#define HTTP_RESP_RESULT                        "result"

#define MFC_SERVICES_JSON_COMMON                "common"
#ifndef MFC_SERVICES_JSON_COMMON_VALUE
#define MFC_SERVICES_JSON_COMMON_VALUE          true
#endif

#define MFC_SERVICES_JSON_NAME                  "name"

#ifndef MFC_SERVICES_JSON_NAME_RTMP_VALUE
//#define MFC_SERVICES_JSON_NAME_RTMP_VALUE       "MyFreeCams RTMP"
#define MFC_SERVICES_JSON_NAME_RTMP_VALUE       "MyFreeCams"
#endif

#ifndef MFC_SERVICES_JSON_NAME_WEBRTC_VALUE
#define MFC_SERVICES_JSON_NAME_WEBRTC_VALUE     "MyFreeCams WebRTC"
#endif

#define MFC_SERVICES_JSON_KEYINT                "keyint"
#ifndef MFC_SERVICES_JSON_KEYINT_VALUE
#define MFC_SERVICES_JSON_KEYINT_VALUE          1
#endif

#define MFC_SERVICES_JSON_BFRAMES               "bframes"
#ifndef MFC_SERVICES_JSON_BFRAMES_VALUE
#define MFC_SERVICES_JSON_BFRAMES_VALUE         0
#endif

#define MFC_SERVICES_JSON_MAX_WIDTH             "max width"
#ifndef MFC_SERVICES_JSON_MAX_WIDTH_VALUE
#define MFC_SERVICES_JSON_MAX_WIDTH_VALUE       1920
#endif

#define MFC_SERVICES_JSON_MAX_HEIGHT            "max height"
#ifndef MFC_SERVICES_JSON_MAX_HEIGHT_VALUE
#define MFC_SERVICES_JSON_MAX_HEIGHT_VALUE      1080
#endif

#define MFC_SERVICES_JSON_MAX_FPS               "max fps"
#ifndef MFC_SERVICES_JSON_MAX_FPS_VALUE
#define MFC_SERVICES_JSON_MAX_FPS_VALUE         60
#endif

#define MFC_SERVICES_JSON_AUDIO_BITRATE         "max audio bitrate"
#ifndef MFC_SERVICES_JSON_AUDIO_BITRATE_VALUE
#define MFC_SERVICES_JSON_AUDIO_BITRATE_VALUE   192
#endif

#define MFC_SERVICES_JSON_VIDEO_BITRATE         "max video bitrate"
#ifndef MFC_SERVICES_JSON_VIDEO_BITRATE_VALUE
#define MFC_SERVICES_JSON_VIDEO_BITRATE_VALUE   10000
#endif

#define MFC_SERVICES_JSON_X264_PROFILE          "profile"
#ifndef MFC_SERVICES_JSON_X264_PROFILE_VALUE
#define MFC_SERVICES_JSON_X264_PROFILE_VALUE    "high"
#endif

#define MFC_SERVICES_JSON_X264OPTS              "x264opts"
#ifndef MFC_SERVICES_JSON_X264OPTS_VALUE
#define MFC_SERVICES_JSON_X264OPTS_VALUE        "tune=zerolatency scenecut=0"
#endif

#define RTMP_SERVICES_FORMAT_VERSION            3

#define SERVICE_JSON_FILE                       "service.json"
#define SERVICE_JSON_SETTING                    "settings"
#define SERVICE_JSON_SERVICE                    "service"
#define SERVICE_JSON_STREAM_KEY                 "key"
#define SERVICE_JSON_STREAM_URL                 "server"

#ifndef MFC_SERVICES_JSON_PRIMARY_SERVER_NAME
#define MFC_SERVICES_JSON_PRIMARY_SERVER_NAME   "Automatic"
#endif
#ifndef MFC_DEFAULT_BROADCAST_URL
#define MFC_DEFAULT_BROADCAST_URL               "rtmp://publish.myfreecams.com/NxServer"
#endif

#ifndef MFC_AUSTRALIA_SERVER_NAME
#define MFC_AUSTRALIA_SERVER_NAME               "Australia"
#endif
#ifndef MFC_AUSTRALIA_BROADCAST_URL
#define MFC_AUSTRALIA_BROADCAST_URL             "rtmp://publish-syd.myfreecams.com/NxServer"
#endif

#ifndef MFC_EAST_ASIA_SERVER_NAME
#define MFC_EAST_ASIA_SERVER_NAME               "East Asia"
#endif
#ifndef MFC_EAST_ASIA_BROADCAST_URL
#define MFC_EAST_ASIA_BROADCAST_URL             "rtmp://publish-tyo.myfreecams.com/NxServer"
#endif

#ifndef MFC_EUROPE_EAST_SERVER_NAME
#define MFC_EUROPE_EAST_SERVER_NAME             "Europe (East)"
#endif
#ifndef MFC_EUROPE_EAST_BROADCAST_URL
#define MFC_EUROPE_EAST_BROADCAST_URL           "rtmp://publish-buh.myfreecams.com/NxServer"
#endif

#ifndef MFC_EUROPE_WEST_SERVER_NAME
#define MFC_EUROPE_WEST_SERVER_NAME             "Europe (West)"
#endif
#ifndef MFC_EUROPE_WEST_BROADCAST_URL
#define MFC_EUROPE_WEST_BROADCAST_URL           "rtmp://publish-ams.myfreecams.com/NxServer"
#endif

#ifndef MFC_NORTH_AMERICA_EAST_SERVER_NAME
#define MFC_NORTH_AMERICA_EAST_SERVER_NAME      "North America (East Coast)"
#endif
#ifndef MFC_NORTH_AMERICA_EAST_BROADCAST_URL
#define MFC_NORTH_AMERICA_EAST_BROADCAST_URL    "rtmp://publish-ord.myfreecams.com/NxServer"
#endif

#ifndef MFC_NORTH_AMERICA_WEST_SERVER_NAME
#define MFC_NORTH_AMERICA_WEST_SERVER_NAME      "North America (West Coast)"
#endif
#ifndef MFC_NORTH_AMERICA_WEST_BROADCAST_URL
#define MFC_NORTH_AMERICA_WEST_BROADCAST_URL    "rtmp://publish-tuk.myfreecams.com/NxServer"
#endif

#ifndef MFC_SOUTH_AMERICA_SERVER_NAME
#define MFC_SOUTH_AMERICA_SERVER_NAME           "South America"
#endif
#ifndef MFC_SOUTH_AMERICA_BROADCAST_URL
#define MFC_SOUTH_AMERICA_BROADCAST_URL         "rtmp://publish-sao.myfreecams.com/NxServer"
#endif

#endif  // MFC_CONFIG_CONSTANTS_H_
