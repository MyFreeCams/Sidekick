#pragma once
#include <cstdint>

// clients must have a version >= x to connect to this server
#define CLIENT_VERSION_REQUIRED     20060925
#define DEFAULT_LOGIN_VERSION       20071025
#define DEFAULT_WEBSOCK_VERSION     20180422

#ifndef LOCALHOST_ADDR
#define LOCALHOST_ADDR              2130706433          // inet_addr('127.0.0.1')
#endif

#define FCMAX_SERVERPACKET          (1024*256)          // 256k limit for server to server packets
#define FCMAX_CLIENTPACKET          8192                // 8k limit for clients per packet

#define PLATFORM_NONE               0
#define PLATFORM_MFC                1
#define PLATFORM_CAMYOU             2
#define PLATFORM_CAMMUNITY          2
#define MAX_PLATFORM                3                   // number of elements when stack allocating arrays for platform
                                                        // specific data, or for validation of platform id value

// hardcoded user id values (FCUSER_TESTCAM1, FCUSER_TRANSCODE, FCUSER_MFCNEWS, etc)
#include <libfcs/FcUserIds.h>

// UserID ranges:
#define USER_ID_START               100                 // 100 being the first valid user id
#define WORKER_ID_START             50000000            // < WORKER_ID_START for users, upto SESSION_ID_START for sessions
#define SESSION_ID_START            75000000            // < SESSION_ID_START is for users + workerids, >75m reserved for session ids
#define CHANNEL_ID_START            100000000
#define SESSCHAN_ID_START           200000000
#define CAMCHAN_ID_START            400000000           // Start for camyou channel ids
#define WREQUEST_ID_START           500000000           // Start for worker request IDs
#define CLUB_ID_START               0
#define CLUBGROUP_ID_START          2000000000          // MfcShare clubs 0..2b, clubgroups 2b+

#define USER_ID_END                 WORKER_ID_START     // (synonym for WORKER_ID_START, when referencing end of user range)
#define WORKER_ID_END               SESSION_ID_START    // (synonym for SESSION_ID_START, when referencing end of worker range)
#define SESSION_ID_END              950000000
#define CHANNEL_ID_END              CAMCHAN_ID_START    // MFC chann id range ends with camyou channel id start
#define SESSCHAN_ID_END             300000000
#define CAMCHAN_ID_END              WREQUEST_ID_START   // Camyou channel id range ends with worker request id range start
#define WREQUEST_ID_END             600000000           // End for worker requestr IDs (cycles in this 100m range)
#define CLUB_ID_END                 CLUBGROUP_ID_START  // End range val for MFCShare Club ids

#define MYWEBCAM_EVERYONE           0
#define MYWEBCAM_ONLYUSERS          1
#define MYWEBCAM_ONLYFRIENDS        2
#define MYWEBCAM_ONLYMODELS         3
#define MYWEBCAM_FRIENDSANDMODELS   4
#define MYWEBCAM_WHITELIST          5
#define MYWEBCAM_FRIEND_ID          100

#define MAX_TOPIC_SZ                160

enum FcAccessOpt : uint64_t
{
    //
    // Mutually exclusive options.  If one of these, can not have other bits set.
    //
    ACL_EXCL_NULL           = (  ZERO64   ),    // No options set. No one can access resource.
    ACL_EXCL_EVERYONE       = (ONE64 <<  0),    // Anyone may access resource
    //
    // Optional options, these are stackable.  One of these must match for client
    // to access resource protected by this ACL option.
    //
    ACL_USERID              = (ONE64 <<  1),    // Is a specific uid (compare determined by feature)
    ACL_USER_BASIC          = (ONE64 <<  2),    // Bit signifies basic users may access resource
    ACL_USER_PREMIUM        = (ONE64 <<  3),    // Bit signifies premium users may access resource
    ACL_USER_MODEL          = (ONE64 <<  4),    // all models may access resource
    ACL_USER_MODEL_SW       = (ONE64 <<  5),    // models that are IsModelSoftware() may access resource
    ACL_USER_ADMIN          = (ONE64 <<  6),    // any admin may access resource
    //
    ACL_RESERVED_7          = (ONE64 <<  7),    // Reserved.
    ACL_RESERVED_8          = (ONE64 <<  8),    // Reserved.
    //
    ACL_OWNER               = (ONE64 <<  9),    // owner of resource may access it
    ACL_FRIENDS             = (ONE64 << 10),    // people that are friends of owner may access
    ACL_CLUBMEMBERS         = (ONE64 << 11),    // people with club memberships from owner may access
    ACL_ROOMHELPERS         = (ONE64 << 12),    // active roomhelpers of owner may access
    ACL_TOPFRIENDS          = (ONE64 << 13),    // top friends of owner may access
    ACL_ROOMMATES           = (ONE64 << 14),    // people in owner's pub room may access
    ACL_ROOMMATES_PVT       = (ONE64 << 15),    // people in owner's pvt, grp, or clubshow room may access
    ACL_WHITELIST           = (ONE64 << 16),    // people in secondary whitelist (list determined by feature)
    //
    ACL_RESERVED_15         = (ONE64 << 17),    // Reserved.
    ACL_RESERVED_16         = (ONE64 << 18),    // Reserved.
    ACL_RESERVED_17         = (ONE64 << 19),    // Reserved.
    ACL_RESERVED_18         = (ONE64 << 20),    // Reserved.
    ACL_RESERVED_19         = (ONE64 << 21),    // Reserved.
    //
    // (Optionally Blocks) Must not be in secondary blacklist
    // List is determined by feature. Only affects access for users
    // who are found in blacklist, they will be rejected.  People not
    // in blacklist must still meet some other criteria in order to
    // qualify for resource (such as being a club member or friend of
    // model with ACL_FRIENDS and ACL_CLUBMEMBERS options)
    //
    ACL_OPT_BLACKLISTED     = (ONE64 << 22),    // people must not be in blacklist.
    ACL_OPT_BANNED          = (ONE64 << 23),    // people must not be banned. Some resources will implicitly
                                                // include this, without having the option specifically set.
    //
    ACL_OPT_RESERVED_24     = (ONE64 << 24),    // Reserved.
    ACL_OPT_RESERVED_25     = (ONE64 << 25),    // Reserved.
    ACL_OPT_RESERVED_26     = (ONE64 << 26),    // Reserved.
    ACL_OPT_RESERVED_27     = (ONE64 << 27),    // Reserved.
    ACL_OPT_RESERVED_28     = (ONE64 << 28),    // Reserved.
    ACL_OPT_RESERVED_29     = (ONE64 << 29),    // Reserved.
    ACL_OPT_RESERVED_30     = (ONE64 << 30),    // Reserved.
    MAX_ACL                 = (ONE64 << 31),
};

enum FcListType
{
    FCL_NULL        = 0,        // Null / Invalid list
    //
    //--/ User-Owned Lists /----//--------------------------------------------------------------
    //
    FCL_FRIENDS,                //  1:  Friends
    FCL_IGNORES,                //  2:  Ignored users
    FCL_BOOKMARKS,              //  3:  Bookmarked Models
    FCL_HIDDEN,                 //  4:  Hidden models (or hidden users)
    FCL_HPFRIENDS,              //  5:  Homepage friends (from profiles)
    FCL_TOPFRIENDS,             //  6:  TopFriends - Models can mark some friends as TopFriends for
    //                          //      moving them to top of lists of friends
    FCL_NEWS_SUBS,              //  7:  Newsfeed Subscriptions a user subscribes to
    FCL_NEWS_HIDDEN,            //  8:  Newsfeed Subscriptions a user hides
    FCL_MYWEBCAM_ALLOW,         //  9:  MyWebcam whitelisted users
    FCL_MYWEBCAM_DENY,          // 10:  MyWebcam blacklisted users
    FCL_BLOCKS_STATES,          // 11:  US/Canadian/Australian State/Provinces a model blocks
    FCL_BLOCKS_COUNTRIES,       // 12:  Contries a model blocks
    FCL_ROOMFILTERS,            // 13:  Models list of either pcre2 or simple substring patterns
    //                          //      for automatic filtering of some cmesgs in their room
    FCL_BANS,                   // 14:  Models list of channel bans
    FCL_MUTES,                  // 15:  Models list of channel mutes
    FCL_UEOPTS,                 // 16:  UEOpt block of data (all non-default UEOpts user has set)
    //                          //
    FCL_RESERVED_17,            // 17:  Reserved/Place Holder for expansion
    FCL_RESERVED_18,            // 18:  Reserved/Place Holder for expansion
    FCL_RESERVED_19,            // 19:  Reserved/Place Holder for expansion
    //                          //
    //--/ Generated Lists /-----//-----------------------------/ Read-Only for Clients /--------
    //                          //
    FCL_TAGS,                   // 20:  tags that models/users add for themselves, client searchable
    FCL_CAMS,                   // 21:  List of models that are online or broadcasting now
    FCL_ROOMMATES,              // 22:  List of people in a specific room
    //                          //
    //--/ Special Purpose /-----//--------------------------------------------------------------
    //                          //
    FCL_SOCIALDATA,             // 23: List marker for user_social_data used in RPC calls. Not
    //                          //     sent to clients in list form.
    //                          //
    //--/ MFCShare Lists /------//--------------------------------------------------------------
    //                          //
    FCL_SHARE_CLUBS,            // 24: List of clubs owned by a model on MFCShare
    FCL_SHARE_CLUBMEMBERSHIPS,  // 25: List of clubs a user is currently a member of
    FCL_SHARE_CLUBSHOWS,        // 26: List of clubshows a user has access to (as member of
    //                          //     constituent club included by show)
    FCL_SHARE_BADGES,           // 27: List of people in this room who have membership to
    //                          //     one or more clubs of this room's model which have
    //                          //     emoji badges associated with them, along with a count
    //                          //     of how many times over they have the badge.
    //                          //
    //--/ Reserved /------------//--------------------------------------------------------------
    //                          //
    FCL_RESERVED_28,            // Reserved for future expansion
    FCL_RESERVED_29,            // Reserved for future expansion
    //                          //
    //--/ End of list /---------//-------------------------------------------------------------
    //                          //
    MAX_FCL                     // Marker for end of FCL_ value range (any valid
    //                          // FcListType is >= FCL_NULL and < MAX_FCL)
};

// Used for sessions mutually exclusive session type, see TkOptType for more descriptive bitmasked values
enum EvSessionType
{
    EVSESSION_NONE                  = 0,
    EVSESSION_PRIVATE               = 1,
    EVSESSION_VOYEUR                = 2,
    EVSESSION_GROUP                 = 3,
    EVSESSION_FEATURE               = 4,        // NOT IMPL
    EVSESSION_AWAYPVT               = 5,        // NO LONGER IMPL
    EVSESSION_CLUB                  = 6,
    EVSESSION_TIP                   = 10,
    EVSESSION_PUBLIC                = 100,
    EVSESSION_AWAY                  = 101,
    EVSESSION_START                 = 102,
    EVSESSION_UPDATE                = 103,
    EVSESSION_STOP                  = 104,
    EVSESSION_SPECIAL               = 1000,
};

// The action type enum describes the type of events taking place in room or state changes of users
enum FcActionType
{
    // FCVIDEO_ values are used as the state code for clients in chat server
    FCVIDEO_TX_IDLE                 = 0,        // Idle
    FCVIDEO_TX_RESET                = 1,        // Model Resetting
    FCVIDEO_TX_AWAY                 = 2,        // Model Away

    FCVIDEO_TX_CONFIRMING           = 11,       // Confirming pvt show with model
    FCVIDEO_TX_PVT                  = 12,       // Private show
    FCVIDEO_TX_GRP                  = 13,       // Group show
    FCVIDEO_TX_CLUB                 = 14,       // Club show

    FCVIDEO_TX_KILLMODEL            = 15,       // Requesting shutdown of model

    FCVIDEO_C2C_ON                  = 20,       // Cam2Cam broadcast started on FMS
    FCVIDEO_C2C_OFF                 = 21,       // Cam2Cam broadcast ended on FMS

    FCVIDEO_RX_IDLE                 = 90,       // Receiver is getting video in free mode
    FCVIDEO_RX_PVT                  = 91,       // Receiver is getting video in pvt
    FCVIDEO_RX_VOY                  = 92,       // Receiver is getting video as a voyeur
    FCVIDEO_RX_GRP                  = 93,       // Receiver is getting video in group
    FCVIDEO_RX_CLUB                 = 94,       // Receiver is getting video in club show

    FCVIDEO_NULL                    = 126,      // Unknown state
    FCVIDEO_OFFLINE                 = 127,      // Offline state

    // FCACT_ values are non-video related to describe actions like joining or parting a room,
    // setting a topic, tipping, etc.
    FCACT_CHAN_TIP                  = 1006,     // Someone tipped (publically) to a model
    FCACT_CHAN_BAN                  = 1011,     // Model banned someone from their room
    FCACT_CHAN_UNBAN                = 1012,     // Model unbanned someone
    FCACT_CHAN_JOIN                 = 1051,     // Joined a room
    FCACT_CHAN_PART                 = 1052,     // Left a room
    FCACT_CHAN_TOPIC                = 1061,     // Model set topic for room

    FCACT_CHAN_WHITEBOARD_ON        = 1101,     // Model turned whiteboard on for given channel
    FCACT_CHAN_WHITEBOARD_OFF       = 1102,     // Model turned whiteboard off for given channel

    //---/ FCACT_ Constants /-----------------------------------------------------------------------
    // codex_sessions actions make use of FCACT.. values (among other services). Specifically
    // in the context of codex_sessions, some additional notes follow.
    //
    FCACT_LOGIN                     = 8001, // client login; issued id/access/refresh tkns
    FCACT_LOGOUT                    = 8002, // client logout; revoke of id/access/refresh tkns
    FCACT_CHECKIN                   = 8003, // most recent check-in w/existing access/id tokens
    FCACT_REFRESH                   = 8004, // client used refresh token to get new access token
    //
    //-----------------------------------------------------------------------/ FCACT_ Constants /---
};

enum FcStreamType : uint64_t
{
    //---/------------------------------------------------------------------------------------------
    //---/ Bits 0..4: Application properties of a stream /------------------------------------------
    //---/------------------------------------------------------------------------------------------
    FVT_NONE                        = (  ZERO64  ),     // Null/Empty/No options set for stream type
    FVT_PUBLIC                      = (ONE64 << 0),     // Model's public video stream
    FVT_PRIVATE                     = (ONE64 << 1),     // Model's private video stream
    FVT_GROUP                       = (ONE64 << 2),     // Model's group video stream
    FVT_MYWEBCAM                    = (ONE64 << 3),     // User's MyWebcam (Cam2Cam) stream
    FVT_CLUB                        = (ONE64 << 4),     // Model's club-only video stream

    //---/------------------------------------------------------------------------------------------
    //---/ Bits 5..6: Reserved for a future application-level stream property /---------------------
    //---/------------------------------------------------------------------------------------------
    FVT_RESERVED_32                 = (ONE64 << 5),     // Reserved option
    FVT_RESERVED_64                 = (ONE64 << 6),     // Reserved option

    //---/------------------------------------------------------------------------------------------
    //---/ Bits 7..11: Access Level of client ctx token is generated for /--------------------------
    //---/------------------------------------------------------------------------------------------
    FVT_ACL_GUEST                   = (ONE64 << 7),     // Client access_level is FCLEVEL_GUEST
    FVT_ACL_BASIC                   = (ONE64 << 8),     // Client access_level is FCLEVEL_BASIC
    FVT_ACL_PREMIUM                 = (ONE64 << 9),     // Client access_level is FCLEVEL_PREMIUM
    FVT_ACL_MODEL                   = (ONE64 << 10),    // Client access_level is FCLEVEL_MODEL
    FVT_ACL_ADMIN                   = (ONE64 << 11),    // Client access_level is FCLEVEL_ADMIN

    //---/------------------------------------------------------------------------------------------
    //---/  Bits 12..16: Privileges and access rights associated with stream /----------------------
    //---/------------------------------------------------------------------------------------------

    //---/ AllowPublish /---------------------------------------------------------------------------
    // Allow publishing this stream. Used typically by the associated model for the stream,
    // but may also be used by transcoder services performing automatic transcoding or
    // relaying/replication of the stream from server to server. This option allows publishing
    // of any variant the server accepts. Typically the model client software will publish the
    // native stream, and transcoding agents will publish variants such as alternate bitrates
    // and resolutions based on the native stream. The AllowPublish privilege grants publishing
    // rights for the native stream as well as any variants or alternates derived from it.
    //
    //---[ Note ]-----------------------------------------------------------------------------------
    //- To allow a client to publish only the transcoded variants of a stream (but not the native  -
    //- stream itself) the AllowTranscode privilege should be used instead.                        -
    //-----------------------------------------------------------------------------------[ Note ]---
    FVT_ALLOW_PUBLISH               = (ONE64 << 12),

    //---/ AllowView /------------------------------------------------------------------------------
    // Allow viewing this stream when model is logged in to chat and in the correct state
    // for the stream (FCVIDEO_TX_IDLE for a FVT_PUBLIC stream, _TX_CLUB for FVT_CLUB, etc.)
    //
    //---[ Note ]-----------------------------------------------------------------------------------
    //- If a model was not logged in, had not attached the stream, or chat state was not in sync   -
    //- with the stream state, the client would not be able to view the stream. To access it in    -
    //- that case, the client must instead have the FVT_ALLOW_PREVIEW privilege.                   -
    //-----------------------------------------------------------------------------------[ Note ]---
    FVT_ALLOW_VIEW                  = (ONE64 << 13),

    //---/ AllowPreview /---------------------------------------------------------------------------
    // Special access which allows viewing a stream or thumbnail snaps regardless of model's
    // chat state. Allows the client bearing the token to view the associated stream even if
    // the stream's model is away, offline, or otherwise in a state that would normally block
    // users from access.  This option is typically set in tokens issued to the model who
    // is publishing the stream, but may also be issued to admins and backend agents such as
    // transcoders, archivers, and replication services.
    //
    //---[ Note ]-----------------------------------------------------------------------------------
    //- Future model helper/room helper features may provide specific users (other than the model) -
    //- this access privilege. A small list of users managed by the model before hand would serve  -
    //- as a VIP list by the chat server when it generated and issued authentication CtxTokens for -
    //- users with rights to that model's streams. Currently the chat sessions for a model's room  -
    //- helper and any additional www or mobile logins by the model herself are NOT granted the    -
    //- AllowPreview privilege in CtxTokens issued. Only the publish token issued to the model     -
    //- ahead of time (used to authorize the initial publishing of a stream) has the AllowPreview  -
    //- privilege included. Because of this, the model client implementations accessing any live   -
    //- previews for their logged in model (thumbnails, video streams, or transcodes of streams)   -
    //- would provide the same publish token credentials with the preview request as the ones used -
    //- in initial publishing of the stream.                                                       -
    //-----------------------------------------------------------------------------------[ Note ]---
    FVT_ALLOW_PREVIEW               = (ONE64 << 14),

    //---/ AllowManage /----------------------------------------------------------------------------
    // Provides management rights to the bearer of token with this permission set. Management
    // rights would allow operations such as terminating a feed being published, start or stop
    // backend services such as archiving, transcoding, or thumbnail generation, or access
    // control for model streams such as granting other user ids or IP address access rights,
    // listing current viewers, blocking viewiers, adding or removing region bans, etc.
    //
    //---[ Note ]-----------------------------------------------------------------------------------
    //- FVT_ALLOW_MANAGE is not currently implemented, and will not provide access to any features -
    //- currently on nginx or wowza servers for associated streams. This is in place now only in   -
    //- anticiptation of future updates providing the implementation.                              -
    //-----------------------------------------------------------------------------------[ Note ]---
    FVT_ALLOW_MANAGE                = (ONE64 << 15),

    //---/ AllowTranscode /-------------------------------------------------------------------------
    // Similar to AllowPublish, but only allows publishing a variant of a native stream. Variants
    // would be any stream with the same prefix, but an additional suffix identifying transcode's
    // bitrate, resolution, and format. Thumbnails or JPEG image snaps created from a native
    // stream are considered transcode variants for the purpose of rights & access control.
    //
    //---[ Note ]-----------------------------------------------------------------------------------
    //- Transcodes may be in container formats that match the native stream (such as WebRTC or     -
    //- FLV/F4V), or in HTTP friendlier formats such as HLS or Dash adaptive streaming. It is not  -
    //- currently possible to limit transcode rights to specific variants or container formats,    -
    //- if the AllowTranscode privilege is granted, the CtxToken is accepted for any transcode     -
    //- variant supported by the server.                                                           -
    //-----------------------------------------------------------------------------------[ Note ]---
    FVT_ALLOW_TRANSCODE             = (ONE64 << 16),

    //---/ AllowMonitorStreams /--------------------------------------------------------------------
    // Allows requesting a list of streams published by model, and list of transcodes active for
    // those streams, along with stats for those streams.
    //
    //---[ Note ]-----------------------------------------------------------------------------------
    //- The AllowMonitorStreams privilege is not currently implemented. Once implemented, video    -
    //- server will process stream monitor requests from clients in a REST-like API interface.     -
    //- Typical uses will be for agents like the OBS plugin, or room helper client software for    -
    //- either room helpers or model software client features that provide the active streams      -
    //- result data returned by REST api in JSON responses.                                        -
    //-----------------------------------------------------------------------------------[ Note ]---
    FVT_ALLOW_MONITOR_STREAMS       = (ONE64 << 17),

    //---/ AllowMonitorClients /--------------------------------------------------------------------
    // Allows requesting a list of clients currently or recently accessing the streams published
    // by model or streams associated with CtxToken including this privilege, along with stats
    // on those clients such as bytes transfered, time client connected or disconnected, etc.
    //
    //---[ Note ]-----------------------------------------------------------------------------------
    //- The AllowMonitorClients privilege is not currently implemented. Once implemented, video    -
    //- server will process client monitor requests from clients in a REST-like API interface.     -
    //- Typical uses will be for agents like the OBS plugin, or model software client features     -
    //- that use client monitor results to track how many viewers the model has per stream,        -
    //- per transcode, and generate aggregate statistics for the model about clients viewing.      -
    //-----------------------------------------------------------------------------------[ Note ]---
    FVT_ALLOW_MONITOR_CLIENTS       = (ONE64 << 18),

    //---/------------------------------------------------------------------------------------------
    //---/  Bits 19..22: Client or transport properties /-------------------------------------------
    //---/------------------------------------------------------------------------------------------

    //---/ ObsRtmp /--------------------------------------------------------------------------------
    // Stream is being published by an OBS client over RTMP.
    //
    FVT_OBS_RTMP                    = (ONE64 << 19),

    //---/ ObsWebRTC /------------------------------------------------------------------------------
    // Stream is being published by an OBS client over WebRTC with OBS plugin.
    // FVT_OBS_WEBRTC_V1 mirrors FCOPT_OBSRTCV1, an OBS formatted stream name with phase code.
    // FVT_OBS_WEBRTC_V2 mirrors FCOPT_OBSRTCV2, a legacy formatted stream name without phase code.
    //
    FVT_OBS_WEBRTC_V1               = (ONE64 << 20),
    FVT_OBS_WEBRTC_V2               = (ONE64 << 21),

    //---/ ModelWebRtmp /---------------------------------------------------------------------------
    // Stream is being published by the ModelWeb or ModelExe app using Flash to publish over RTMP
    //
    FVT_MWEB_RTMP                   = (ONE64 << 22),

    //---/ ModelWebWebRTC /-------------------------------------------------------------------------
    // Stream is being published by ModelWeb using WebRTC to publish stream. MWEB published
    // WebRTC streams mirror FCOPT_WEBRTCV1 model flag option, and stream names for this streamt
    // do not include a phase code.
    //
    FVT_MWEB_WEBRTC                 = (ONE64 << 23),
};

// A macro for all of the ACL permissions related to access level.
#define FVT_ACL_ALL               ( FVT_ACL_GUEST | FVT_ACL_BASIC | FVT_ACL_PREMIUM | FVT_ACL_MODEL | FVT_ACL_ADMIN )

enum FcServType
{
    //---//------------------------------------------------------------------------------------//---
    //---/  Video Server Types for servers that accept published streams from models/users      /---
    //---/  Interchangeable with FCAPP_ types with same values + 500                            /---
    //---//------------------------------------------------------------------------------------//---
    //
    //---/ Flash/RTMP Legacy Servers /--------------------------------------------------------------
    //
    VST_FMS                         = 0,        // server_config.type_data = 0 for FMS servers
    //
    //
    //---/ Wowza + RTMP Syntax/Events /-------------------------------------------------------------
    //
    VST_WOWZA                       = 1,        // 1 for wowza servers in normal model video model
    VST_WOWZA_RELAY                 = 2,        // Relay mode type wowza (tcp proxy) servers
    VST_WOWZA_CAM2CAM               = 3,        // Cam2Cam servers on wowza
    //
    //
    //---/ Wowza + OBS Syntax/Events /--------------------------------------------------------------
    //
    VST_WOWZA_OBSRTC_V1             = 6,        // Wowza w/WebRTC: OBS Syntax & Events
    VST_WOWZA_OBSRTC_V2             = 7,        // Wowza w/WebRTC: RTMP Syntax & Events
    //
    // Wowza OBS servers (like nginx but not using all of nginx services, for example no client-id
    // published & auth using standard wowza fmt instead of /x-hls/ or /x-dsh/ for segmented streams
    //
    VST_WOWZA_OBS                   = 8,
    //
    //---/ NGINX + OBS Syntax/Events /--------------------------------------------------------------
    //
    // NGINX server accepting RTMP streams (usually meaning OBS publishing clients) sending events
    // to an EdgeChat instance through HTTP based notification calls using the /rtmpx/ API prefix.
    //
    VST_NGINX                       = 10,
    VST_NULL                        = 255,      // invalid/empty value for server type
    //
    //---//------------------------------------------------------------------------/ FcServType /---
};

// Options to describe a stream.  Most are optional, some are mutually exclusive (for example
// only one bit from each major grouping should be set, most likely)
enum FcStreamOpt : uint64_t
{
    FCV_EMPTY                       = (  ZERO64  ),     // Null/Empty/No options set for encoder type
    //
    // Application source of stream
    //
    FCV_ENCODER_FME                 = (ONE64 << 0),     // Flash Media Encoder
    FCV_ENCODER_OBS                 = (ONE64 << 1),     // Open Broadcast System
    FCV_ENCODER_FLASH               = (ONE64 << 2),     // Flash SWF in Browser/ModelExe encoding
    FCV_ENCODER_FFMPEG              = (ONE64 << 3),     // ffmpeg, probably a stream relay or archive process
    FCV_ENCODER_BROWSER             = (ONE64 << 4),     // Chrome,IE,Safari browser, i.e. for WebRTC broadcasts
    //
    // bits 5 reserved for more encoder types
    //
    // Server/Relay process for stream
    FCV_SERVER_FMS                  = (ONE64 << 6),     // FMS serving stream
    FCV_SERVER_WOWZA                = (ONE64 << 7),     // Wowza serving stream
    FCV_SERVER_NGINX                = (ONE64 << 8),     // nginx-rtmp serving stream
    FCV_SERVER_WEB                  = (ONE64 << 9),     // Normal webserver serving stream
    //
    FCV_RESERVED_RES10              = (ONE64 << 10),    // Reserved
    FCV_RESERVED_RES11              = (ONE64 << 11),    // Reserved
    //
    // Proxies that may be part of stream
    FCV_PROXY_NGINX                 = (ONE64 << 12),
    FCV_PROXY_CUSTOM                = (ONE64 << 13),
    //
    FCV_RESERVED_RES14              = (ONE64 << 14),    // Reserved
    //

    //
    //--/ Endpoint application consuming stream
    FCV_DECODER_BROWSER             = (ONE64 << 15),    // Client Web browser decoding & rendering directly
    FCV_DECODER_FLASH               = (ONE64 << 16),    // Client Web browser decode & render w/flash SWF plugin
    FCV_DECODER_FFMPEG              = (ONE64 << 17),    // Server using ffmpeg for transcode, relay, or archive
    FCV_DECODER_VLC                 = (ONE64 << 18),    // VLC Desktop Application decoding
    FCV_DECODER_CUSTOM              = (ONE64 << 19),    // Custom application accepts stream for decode/archive
    //
    //---/ Reserved Options 20..24
    FCV_RESERVED_RES20              = (ONE64 << 20),    // Custom application accepts stream for decode/archive
    FCV_RESERVED_RES21              = (ONE64 << 21),    // Custom application accepts stream for decode/archive
    FCV_RESERVED_RES22              = (ONE64 << 22),    // Custom application accepts stream for decode/archive
    FCV_RESERVED_RES23              = (ONE64 << 23),    // Custom application accepts stream for decode/archive
    FCV_RESERVED_RES24              = (ONE64 << 24),    // Custom application accepts stream for decode/archive
    //
    //---/ INBOUND Transport /----------------------------------------------------------------------
    // Transport/Streaming Protocol of stream for INBOUND (Servers advertising accepted
    // transport means for published streams, or existing streams describing the format
    // they are publishing in even if the output format has been transmuxed to another
    // format.
    //
    FCV_IN_TRANS_WEBRTC            = (ONE64 << 25),    // WebRTC Streaming (TCP/UDP/SIP)
    FCV_IN_TRANS_RTMP              = (ONE64 << 26),    // Adobe Real Time Messaging Protocol (FMS/Wowza)
    FCV_IN_TRANS_RTSP              = (ONE64 << 27),    // Open RTSP/RTP protocols
    FCV_IN_TRANS_JPEG              = (ONE64 << 28),    // JPEG Push (or single frame image)
    //
    FCV_IN_TRANS_RES29             = (ONE64 << 29),    // Reserved
    FCV_IN_TRANS_RES30             = (ONE64 << 30),    // Reserved
    FCV_IN_TRANS_RES31             = (ONE64 << 31),    // Reserved
    FCV_IN_TRANS_RES32             = (ONE64 << 32),    // Reserved
    //
    //
    //---/ OUTBOUND Transport /---------------------------------------------------------------------
    // Transport/Streaming Protocol of stream for the OUTBOUND (A stream that can be viewed
    // may have these bits set for available transmuxed or transcoded formats. A client may
    // advertise which transport formats they can work with to view streams in with these
    // options. A specific option may be recorded in logs to show what type of stream happened
    // to be viewed for a session of some kind.
    //
    FCV_OUT_TRANS_WEBRTC            = (ONE64 << 33),    // WebRTC Streaming (TCP/UDP/SIP)
    FCV_OUT_TRANS_RTMP              = (ONE64 << 34),    // Real Time Messaging Protocol
    FCV_OUT_TRANS_RTSP              = (ONE64 << 35),    // Open RTSP/RTP protocols
    FCV_OUT_TRANS_JPEG              = (ONE64 << 36),    // JPEG Push (or single frame image)
    FCV_OUT_TRANS_HDS               = (ONE64 << 37),    // Adobe HTTP Dynamic Streaming
    FCV_OUT_TRANS_HLS               = (ONE64 << 38),    // Apple HTTP Live Streaming
    FCV_OUT_TRANS_DASH              = (ONE64 << 39),    // MPEG-DASH Streaming (Like HDS/HLS)
    //
    FCV_OUT_TRANS_RES40             = (ONE64 << 40),    // Reserved
    FCV_OUT_TRANS_RES41             = (ONE64 << 41),    // Reserved
    FCV_OUT_TRANS_RES42             = (ONE64 << 42),    // Reserved
    //
    //
    //---/ Encoded/Format Containers /--------------------------------------------------------------
    // Format types of the stream (either the output format available or input format accepted).
    //
    FCV_FMT_MP4                     = (ONE64 << 43),    // MPEG-4 A/V Container for (typically) H.264 encoded media
    FCV_FMT_FLV                     = (ONE64 << 44),    // Flash Video
    FCV_FMT_MP3                     = (ONE64 << 45),    // MPEG-3 Audio Format
    FCV_FMT_WMV                     = (ONE64 << 46),    // MPEG-3 Audio Format
    FCV_FMT_JPG                     = (ONE64 << 47),    // MPEG-3 Audio Format
    FCV_FMT_M3U8                    = (ONE64 << 48),    // Using m3u8 segmented format (for HLS, HDS, DASH, etc)
    //
    FCV_FMT_RES49                   = (ONE64 << 49),    // Reserved
    FCV_FMT_RES50                   = (ONE64 << 50),    // Reserved
    //
    // Codec used for video in stream
    //
    FCV_VCODEC_H264                 = (ONE64 << 51),    // H.264 MPEG-4 Video Codec
    FCV_VCODEC_VP6                  = (ONE64 << 52),    // On2 / Google VP6 Format
    FCV_VCODEC_VP8                  = (ONE64 << 53),    // On2 / Google VP6 Format
    FCV_VCODEC_OGG                  = (ONE64 << 54),    // Ogg video format
    //
    FCV_VCODEC_RES55                = (ONE64 << 55),    // Reserved
    FCV_VCODEC_RES56                = (ONE64 << 56),    // Reserved
    //
    // Codec used for audio in stream
    FCV_ACODEC_AAC                  = (ONE64 << 57),    // AAC Audio Encoding
    FCV_ACODEC_MP3                  = (ONE64 << 58),    // MP3 Audio Encoding format
    FCV_ACODEC_WMA                  = (ONE64 << 59),    // MP3 Audio Encoding format
    FCV_ACODEC_SPEEX                = (ONE64 << 60),    // Speex audio format
    FCV_ACODEC_VORBIS               = (ONE64 << 61),    // Vorbis audio format
    //
    FCV_ACODEC_RES62                = (ONE64 << 62),    // Reserved
    FCV_ACODEC_RES63                = (ONE64 << 63),    // Reserved
};


// Convert a model's user id to a public channel id for that model
#define USER_TO_CHANNEL(n)          (n <  CHANNEL_ID_START ? n + CHANNEL_ID_START : n)

// Convert a MFC platform model's uid to session channel (grp/pvt)
#define USER_TO_SESS(n)             (n <  CHANNEL_ID_START ? n + SESSCHAN_ID_START : n)

// Convert a model's public channel id to their private/group channel id
#define CHANNEL_TO_SESS(n)          ((n >= CHANNEL_ID_START && n < SESSCHAN_ID_START) ? n + CHANNEL_ID_START : n)

// Extract a model's user id from their private/group channel id
#define USER_FROM_SESS(n)           (n >= SESSCHAN_ID_START ? n - SESSCHAN_ID_START : n)

// Extract a model's user id from their public channel id
#define USER_FROM_CHANNEL(n)        (n >= CHANNEL_ID_START ? n - CHANNEL_ID_START : n)

// Extract a model's public channel id from their private/group channel id
#define CHANNEL_FROM_SESS(n)        (n >= SESSCHAN_ID_START ? n - CHANNEL_ID_START : n)

// Test if an id is a private/group channel id
#define IS_SESS_CHANNEL(n)          (n >= SESSCHAN_ID_START && n < SESSCHAN_ID_END)

// Test if an id is a public channel id
#define IS_USER_CHANNEL(n)          (n >= CHANNEL_ID_START && n < SESSCHAN_ID_START)

// Test if an id is a camyou channel id
#define IS_CAM_CHANNEL(n)           (n >= CAMCHAN_ID_START && n < CAMCHAN_ID_END)

// Extract a model's user id from their camyou channel id
#define USER_FROM_CAMCHAN(n)        (n >= CAMCHAN_ID_START ? n - CAMCHAN_ID_START : n)

// Convert a model's user id to the camyou channel id for that model
#define USER_TO_CAMCHAN(n)          (n < USER_ID_END ? n + CAMCHAN_ID_START : n)

// Determine if clubid is a clubgroup or a regular group
#define IS_CLUB_GROUP(n)            (n >= CLUBGROUP_ID_START)

#define FCPROTOCOL_MAGIC            0x8722aab2
#define FCPROTOCOL_MAGIC_NETORDER   0xb2aa2287  // htonl(FCPROTOCOL_MAGIC)
#define FCWPROT_MAGIC               0x6288ccdd

#define FCPROT_JSOCKET              'j'         // Java or native socket connection direct to server
#define FCPROT_FLASHSOCKET          'f'         // Flashsocket (XMLSocket in flash player) direct to server
#define FCPROT_AJAX                 'a'         // AJAX connection simulating socket through ZAjaxGW in zeus
#define FCPROT_WEBSOCKET            'w'         // Websocket simulating socket through mod_websocket in lighttpd
#define FCPROT_LIBWEBSOCK           'l'         // Native websocket directly terminated via libwebsockets

// Basic commands
#define FCTYPE_NULL                 0
#define FCTYPE_LOGIN                1
#define FCTYPE_ADDFRIEND            2
#define FCTYPE_PMESG                3
#define FCTYPE_STATUS               4
#define FCTYPE_DETAILS              5
#define FCTYPE_TOKENINC             6
#define FCTYPE_ADDIGNORE            7
#define FCTYPE_PRIVACY              8
#define FCTYPE_ADDFRIENDREQ         9
#define FCTYPE_USERNAMELOOKUP       10
#define FCTYPE_ZBAN                 11
#define FCTYPE_BROADCASTNEWS        12
#define FCTYPE_ANNOUNCE             13
#define FCTYPE_MANAGELIST           14
#define FCTYPE_INBOX                15
#define FCTYPE_GWCONNECT            16
#define FCTYPE_RELOADSETTINGS       17
#define FCTYPE_HIDEUSERS            18
#define FCTYPE_RULEVIOLATION        19
#define FCTYPE_SESSIONSTATE         20
#define FCTYPE_REQUESTPVT           21
#define FCTYPE_ACCEPTPVT            22
#define FCTYPE_REJECTPVT            23
#define FCTYPE_ENDSESSION           24
#define FCTYPE_TXPROFILE            25
#define FCTYPE_STARTVOYEUR          26
#define FCTYPE_SERVERREFRESH        27
#define FCTYPE_SETTING              28
#define FCTYPE_BWSTATS              29
#define FCTYPE_TKX                  30
#define FCTYPE_SETTEXTOPT           31
#define FCTYPE_SERVERCONFIG         32
#define FCTYPE_MODELGROUP           33
#define FCTYPE_REQUESTGRP           34
#define FCTYPE_STATUSGRP            35
#define FCTYPE_GROUPCHAT            36
#define FCTYPE_CLOSEGRP             37
#define FCTYPE_UCR                  38
#define FCTYPE_MYUCR                39
#define FCTYPE_SLAVECON             40
#define FCTYPE_SLAVECMD             41
#define FCTYPE_SLAVEFRIEND          42
#define FCTYPE_SLAVEVSHARE          43
#define FCTYPE_ROOMDATA             44
#define FCTYPE_NEWSITEM             45
#define FCTYPE_GUESTCOUNT           46
#define FCTYPE_PRELOGINQ            47
#define FCTYPE_MODELGROUPSZ         48
#define FCTYPE_ROOMHELPER           49
#define FCTYPE_CMESG                50
#define FCTYPE_JOINCHAN             51
#define FCTYPE_CREATECHAN           52
#define FCTYPE_INVITECHAN           53
#define FCTYPE_RPC                  54
#define FCTYPE_QUIETCHAN            55
#define FCTYPE_BANCHAN              56
#define FCTYPE_PREVIEWCHAN          57
#define FCTYPE_SHUTDOWN             58
#define FCTYPE_LISTBANS             59
#define FCTYPE_UNBAN                60
#define FCTYPE_SETWELCOME           61
#define FCTYPE_CHANOP               62
#define FCTYPE_LISTCHAN             63
#define FCTYPE_TAGS                 64
#define FCTYPE_SETPCODE             65
#define FCTYPE_SETMINTIP            66
#define FCTYPE_UEOPT                67
#define FCTYPE_HDVIDEO              68
#define FCTYPE_METRICS              69
#define FCTYPE_OFFERCAM             70
#define FCTYPE_REQUESTCAM           71
#define FCTYPE_MYWEBCAM             72
#define FCTYPE_MYCAMSTATE           73
#define FCTYPE_PMHISTORY            74
#define FCTYPE_CHATFLASH            75
#define FCTYPE_TRUEPVT              76
#define FCTYPE_BOOKMARKS            77
#define FCTYPE_EVENT                78
#define FCTYPE_STATEDUMP            79
#define FCTYPE_RECOMMEND            80
#define FCTYPE_EXTDATA              81
#define FCTYPE_ADMINEXT             82
#define FCTYPE_SESSEVENT            83
#define FCTYPE_NOTIFY               84
#define FCTYPE_PUBLISH              85
#define FCTYPE_XREQUEST             86
#define FCTYPE_XRESPONSE            87
#define FCTYPE_EDGECON              88
#define FCTYPE_XMESG                89
#define FCTYPE_CLUBSHOW             90
#define FCTYPE_CLUBCMD              91
#define FCTYPE_AGENT                92
#define FCTYPE_ZGWINVALID           95
#define FCTYPE_CONNECTING           96
#define FCTYPE_CONNECTED            97
#define FCTYPE_DISCONNECTED         98
#define FCTYPE_LOGOUT               99


// Send a message from server to server of type FCTYPE_RPC, set dwArg1 to a value in FcRpcType.
// dwFrom and dwTo are server IDs, Payload is FCMSG + extra bytes (if dwMsgLen > 0 in FCMSG)
enum FcRpcType
{
    FCRPC_NONE = 0,                     // Null or no-op RPC code/id

    //-- General Purpose FCServer/EdgeChat RPCs ----------------------------------------------------------------------------
    //
    // These RPCs are used mainly between FCServer and EdgeChat
    //

    FCRPC_UPDATEFRIEND,                 // Call FcExt::Core::UpdateFriendMap() with from,to,arg1 from FCMSG attachment as user,
                                        // friend to add/remove, op (join/part)

    FCRPC_UPDATEIGNORE,                 // Call FcExt::Core::UpdateIgnoreMap() with from,to,arg1 from FCMSG attachment as user,
                                        // friend to ignore/unignore, op (join/part)

    FCRPC_RESLOADED,                    // Used to tell another server that a resource is loaded in cache or SQL they can load
                                        // directly (such as friend list, ignore list, bans, etc).  Current list of supported
                                        // resource types:
                                        //
                                        // dwArg1: Session ID of user who just loaded a resource
                                        //  dwArg2: List or resource id as identified by FCTYPE_ADDFRIEND, FCTYPE_ADDIGNORE, or
                                        // FCTYPE_BANCHAN.


    //-- General Purpose Worker RPCs --------------------------------------------------------------------------------------------
    //
    // These RPCs are used mainly between FCWorker3 and chat servers (either FCServer or EdgeChat, core or module code)
    //

    FCRPC_W_READY,                      // Sent to master by worker to indicate they are ready to accept a request.
                                        // TODO dwArg1 to indicate capabilities of worker (such as processing userdata cache)

    FCRPC_W_OFFLINEQUERY,               // Sent to worker by chat server to run offline query.  No response will be returned to server.
                                        // arg1 indicates the database connection profile to use, and the payload is the query to
                                        // execute.  Query should be escaped/SQL safe already, no escaping is done by the worker.
                                        //
                                        // dwArg1: Database ID described by FcExt::DBType enum (DB_ARES, DB_PM, etc)
                                        //

    FCRPC_W_FRIENDLIST,                 // Sent to worker to collect friendlist data for a given user (request mode) or sent to
                                        // server by worker to indicate the job completed (response mode).
                                        //
                                        // dwArg1: 0, not used
                                        // dwArg2: Request ID
                                        // dwFrom: [response only: FCRESPONSE_SUCCESS on sucess or FCRESPONSE_ERROR for failure]
                                        //
                                        // Payload: FCW_REQUEST structure with FCRPC, session id, etc embedded

    FCRPC_W_IGNORELIST,                 // Sent to worker to collect ignorelist data for a given user (request mode) or sent to
                                        // server by worker to indicate the job completed (response mode).
                                        //
                                        // dwArg1: 0, not used
                                        // dwArg2: Request ID
                                        // dwFrom: [response only: FCRESPONSE_SUCCESS on sucess or FCRESPONSE_ERROR for failure]
                                        //
                                        // Payload: FCW_REQUEST structure with FCRPC, session id, etc embedded


    //-- Extension Specific Worker RPCs ------------------------------------------------------------------------------------------
    //
    // These RPCs are for modules running on FCServer or EdgeChat to make out of process requests of FCWorker3.  FCWorker3
    // also will have the module loaded, although will use the FcExt::ExtensionWorker interface instead of the FcExt::Extension
    // interface.
    //

    FCRPC_W_EXT_REQUEST,                // Sent to a worker by master to queue a new job.  arg1 is extension id, arg2 is job id.
                                        // payload is unique to each extension but provides the necessary query or request data
                                        // for the workers module of the same extension ID to perform.  Extensions using FCWorker
                                        // RPC services will need to implement an ExtensionWorker interface that FCWorker can
                                        // create to handle and process this request.

    FCRPC_W_EXT_RESPONSE,               // Sent to master by worker as a response to a previous request.  arg1 is the extension id
                                        // of the extension making and responding to the request, arg2 is the request ID given to
                                        // this job when it was delivered to the worker, payload is unique to each extension
                                        // but will be decipherable by the extension handling the response, and dwFrom will either
                                        // be 0 or 1.  When dwFrom is 0, there are no more response packets coming, and server can
                                        // mark this job as finished.  When dwFrom is 1 there are additional packets coming as part
                                        // of the response.  Some responses to requests may be large and take many packets to send
                                        // in it's entirety, with the dwFrom value the server can optionally queue up that response
                                        // before handing it off to the originating extension.

    FCRPC_FCSVAR,                       // Sent from any server to any other FCS server to propogate changes to FcsVar json
                                        // variable states.
                                        //
                                        // arg1 is the op (FCCHAN_JOIN, FCCHAN_PART, FCCHAN_JOIN|FCCHAN_PART,
                                        // or FCCHAN_LIST).  When arg1 is FCCHAN_LIST, the JSON data is the entire FcsVar object,
                                        // rather than just the delta.  This RPC with op of FCCHAN_LIST and an empty payload
                                        // is sent as a request for the current FcsVar object to be sent back (in another
                                        // RPC call, with dwArg1 of FCCHAN_LIST again but hopefully a non-empty JSON payload)
                                        //
                                        // arg2 is context id (reserved, must be 0).
                                        //
                                        // Payload is JSON object of changes or array of keys to be removed.

    FCRPC_W_REGISTER,                   // Sent to master by worker when worker wants to register to handle user input
                                        // types. dwFrom is FCTYPE_ value to filter for any users.  dwArg1 and dwArg2
                                        // are matching values for which messages to pass through, if dwArg1 or dwArg2 is
                                        // 0, any matching messages are passed on to worker, if non-zero then only matching
                                        // messages are passed to worker for processing. dwTo indicates if the input queue
                                        // should be blocked while processing the message (1 to block, 0 to not block).



};

// Bitmask for 'opts' column of spamfilterDB's patterns and logs.  opts of 0, or
// ROOMFILTER_EMPTY, means the rule is disabled
enum FcRoomFilterType
{
    ROOMFILTER_EMPTY                = (  ZERO64  ), // No action - do nothing, no options chosen for matches

    //
    // Room filter actions - each option set here will result in different behaviors
    // when processing FCTYPE_CMESGs and testing patterns with it.
    //
    ROOMFILTER_LOG                  = (ONE64 << 0), // Log any matches in spamfilter_logs
    ROOMFILTER_DROP                 = (ONE64 << 1), // Drop any matching CMESGs
    ROOMFILTER_WARN                 = (ONE64 << 2), // Send message back to client blacklist event
    //
    // Filter expression is a PCRE2 compat expr, if NOT set then is simple substring
    ROOMFILTER_REGEX                = (ONE64 << 3),
    //
    // Filter was created by an admin, not the model -- do not show to model, should
    // the model_id for this pattern be non-zero.  If model_id is 0, the pattern will
    // match messages for ALL rooms, but only when this bit is set will it apply to
    // every room. arg1 of sf_patterns should be the employee_id who created the rule.
    ROOMFILTER_SPECIAL              = (ONE64 << 4),
    //
    // Filter was created by a room helper, not by model themselves.
    ROOMFILTER_FROMHELPER           = (ONE64 << 5),
    //
    // Exact word match - filter is a substring type filter, but exact match on words,
    // will not match messages where the pattern is surrounded by other letters
    // (surrounded by whitespace, EOL, SOL, or punctuation will match though)
    ROOMFILTER_EXACT_WORD           = (ONE64 << 6),
    //
    ROOMFILTER_RESERVED_7           = (ONE64 << 7), // Reserved action bit - not used yet
    ROOMFILTER_RESERVED_8           = (ONE64 << 8), // Reserved action bit - not used yet
    ROOMFILTER_RESERVED_9           = (ONE64 << 9), // Reserved action bit - not used yet
    //
    //
    // Room filter matching requirements - additional options to narrow when a match is made.
    // By default, these should all be set.  Removing any of these bits will mean less matches for this
    // rule (by excluding any instances where one of these is true but not set in the opts)
    //
    //
    // Access level of person sending a mesg that might need to be filtered.  Example: if
    // pattern only has ROOMFILTER_IS_BASIC set, when a guest, premium, or model sends a
    // message, this rule is not tested since it's pattern should only be searched for when
    // sender is accesslevel FCLEVEL_BASIC.  Premiums sending messages that match this pattern
    // would have no filter, warn, or log effect if ROOMFILTER_IS_PREMIUM wasn't also set in
    // the options bitmask set of values.
    //
    ROOMFILTER_IS_GUEST             = (ONE64 << 10),    // Match when sender is a guest
    ROOMFILTER_IS_BASIC             = (ONE64 << 11),    // Match when sender is a basic
    ROOMFILTER_IS_PREMIUM           = (ONE64 << 12),    // Match when sender is a premium
    ROOMFILTER_IS_MODEL             = (ONE64 << 13),    // Match when sender is a model
    //
    // If neither of these are set, rule will only match messages for UCR rooms which don't
    // exist in the 100m+ range (normally one or both of these should be set)
    ROOMFILTER_IN_PUBLIC            = (ONE64 << 14),    // Match when in models pub channel
    ROOMFILTER_IN_SESSION           = (ONE64 << 15),    // Match when in either Grp or Pvt channel.
    //
    ROOMFILTER_RESERVED_16          = (ONE64 << 16),    // Reserved match bit - not used yet
    ROOMFILTER_RESERVED_17          = (ONE64 << 17),    // Reserved match bit - not used yet
    ROOMFILTER_RESERVED_18          = (ONE64 << 18),    // Reserved match bit - not used yet
    ROOMFILTER_RESERVED_19          = (ONE64 << 19),    // Reserved match bit - not used yet
    ROOMFILTER_RESERVED_20          = (ONE64 << 20),    // Reserved match bit - not used yet
};



#define FCRESPONSE_SUCCESS              0           // Some action succeeded
#define FCRESPONSE_ERROR                1           // Some action failed
#define FCRESPONSE_NOTICE               2           // Some action succeeded but with a notice (such as away message)
#define FCRESPONSE_SUSPEND              3           // Login notice of account suspension for rule violation
#define FCRESPONSE_SHUTOFF              4           // Login notice of account shutoff for rule violation, or in username lookup for inactive account
#define FCRESPONSE_WARNING              5           // Login notice (warning only) of some rule violation
#define FCRESPONSE_QUEUED               6           // Some action has been queued and may be completed later
#define FCRESPONSE_NO_RESULTS           7           // No data associated with otherwise successful request
#define FCRESPONSE_CACHED               8           // Success, although response data was stored in redis or memcached
                                                    // For clients, it indicates the payload of the packet is a FCW_EXTWORK object serialized
                                                    // to:  {"respkey":1154327814,"type":2,"opts":256} for dwRespKey [unique key id], dwType
                                                    //       [FCTYPE_ value], qwOpts [64bit FCWOPT_ mask]
#define FCRESPONSE_JSON                 9           // Payload data has json object instead of simple string

#define FCRESPONSE_INVALIDUSER          10          // Can't perform that action due to invalid target (e.g. add friend of invalid user name or id)
#define FCRESPONSE_NOACCESS             11          // Can't perform that action due to lack of access (e.g. ignore an admin)
#define FCRESPONSE_NOSPACE              12          // No room in resource container for whtever action was attempting (i.e. adding friend
                                                    // or ignore past MAX_USERLIST_SZ)
#define FCRESPONSE_INVALIDREQ           13          // Invalid or unparseable request syntax (typically missing or invalid json object)
#define FCRESPONSE_INVALIDARG           14          // Invalid or out of range argument in request (e.g. userID of 3 or >4billion, sessionID in userID range)
#define FCRESPONSE_NOTFOUND             15          // Target of action or request valid, but not found (e.g. userID not online, resource
                                                    // such as channel name does not exist)
#define FCRESPONSE_INSUFFICIENT         16          // Response in
#define FCRESPONSE_EXPIRED              17          // result or data was too old to be successfully used
#define FCRESPONSE_BINARY               18          // payload data is encoded in packed struct binary format (struct definition dependant on
                                                    // overall type of message, for example FCTYPE_PUBLISH events in FCSMSG packets from server
                                                    // to server, FCRESPONSE_BINARY indicates chMsg is a VIDEO_FEEDSTATE_EVENT structure)
#define FCRESPONSE_UNKNOWN              255         // Undetermined / Unknown error, usually used by server as an intermediate value
                                                    // to designate uninitialized or unset error value, and set before returning to client.

#define FCWINDOW_CAMWATCHERS            33

// Defines the access level of a user (UserData::m_root.nLevel, or in sql euDB.users.access_level)
enum FcAccessLevel
{
    FCLEVEL_INVALID                     = -1,       // access_level for invalid / unauthenticated clients
    FCLEVEL_GUEST                       = 0,        // access_level for Guests
    FCLEVEL_BASIC                       = 1,        // access_level for Basic Members
    FCLEVEL_PREMIUM                     = 2,        // access_level for Premium Members
    FCLEVEL_MODEL                       = 4,        // access_level for Models
    FCLEVEL_ADMIN                       = 5         // access_level for Admins
};

enum FcChanVal
{
    FCCHAN_NOOPT                        = ( 0 << 0  ),  // 0:       No options associated
    FCCHAN_JOIN                         = ( 1 << 0  ),  // 1:       User is joining channel, or general purpose "add" op
    FCCHAN_PART                         = ( 1 << 1  ),  // 2:       User is parting channel, or general purpose "remove" op
    FCCHAN_OLDMSG                       = ( 1 << 2  ),  // 4:       Message was sent before you connected to this channel (for graying out)
    FCCHAN_HISTORY                      = ( 1 << 3  ),  // 8:       Requesting join channel with recent history option
    FCCHAN_LIST                         = ( 1 << 4  ),  // 16:      Used by some modules as a dwOp mode in relation to _JOIN and _PART for listing
    FCCHAN_WELCOME                      = ( 1 << 5  ),  // 32:      If on arg2 of cmesg, then cmesg is the welcome message for a room
    FCCHAN_BATCHPART                    = ( 1 << 6  ),  // 64:      Used in friend list batch deletes
    FCCHAN_EXT_USERNAME                 = ( 1 << 7  ),  // 128:     ext_managelists requests expect abbreviated username/last login data for responses
    FCCHAN_EXT_USERDATA                 = ( 1 << 8  ),  // 256:     ext_managelists requests expect full user/session data object when available, otherwise EXT_USERNAME
    FCCHAN_NO_CHANGES                   = ( 1 << 9  ),  // 512:     general purpose no changes necessary op
    FCCHAN_APPEND                       = ( 1 << 10 ),  // 1024:    general purpose append op
    FCCHAN_REPLACE                      = ( 1 << 11 ),  // 2048:    general purpose replace op
    FCCHAN_UPDATE                       = ( 1 << 12 ),  // 4096:    general purpose update op
    FCCHAN_QUERY                        = ( 1 << 13 ),  // 8192:    general purpose query/request details op
    FCCHAN_EXPIRE                       = ( 1 << 14 ),  // 16384:   general purpose expire op
    FCCHAN_NOTIFY                       = ( 1 << 15 ),  // 32768:   general purpose notify op
    FCCHAN_APPLY                        = ( 1 << 16 ),  // 65536:   general purpose apply op
    FCCHAN_IMPORT                       = ( 1 << 17 ),  // 131072:  general purpose import op
    FCCHAN_BATCH                        = ( 1 << 18 ),  // 262144:  general purpose modifier designating batch operation (vs single operation)
    FCCHAN_TRUNCATE                     = ( 1 << 19 ),  // 524288:  general purpose truncate op (opposite of append op)
};

#define FCCHAN_ERR_NOCHANNEL            2           // cmesg or join failed, no channel found of that id
#define FCCHAN_ERR_NOTMEMBER            3           // cmesg failed, not a member of that channel
#define FCCHAN_ERR_GUESTMUTE            4           // cmesg failed, you are a guest and guest mute is on
#define FCCHAN_ERR_GROUPMUTE            5           // cmesg failed, you are a guest/basic chatting in a group chat session
#define FCCHAN_ERR_NOTALLOWED           6           // join failed, not allowed in room (although not a ban)

#define FCGROUP_NONE                    0           // no error code/reason related to groupchat session update
#define FCGROUP_EXPIRED                 1           // A group chat initiation has expired
#define FCGROUP_BUSY                    2           // A group chat initiation join request denied; in process of state change to session
#define FCGROUP_EMPTY                   3           // A group chat initiation was deserted, no users left! shutting down
#define FCGROUP_DECLINED                4           // A group chat request was declined by the model
#define FCGROUP_UNAVAILABLE             5           // Model not found, not broadcasting, or administratively prohibited from group chat

#define FCGROUP_SESSION                 9           // Transitioning from group chat initiation to group chat session

#define FCACCEPT_NOBODY                 0
#define FCACCEPT_FRIENDS                1
#define FCACCEPT_ALL                    2
#define FCACCEPT_V2_NONE                8
#define FCACCEPT_V2_FRIENDS             16
#define FCACCEPT_V2_MODELS              32
#define FCACCEPT_V2_PREMIUMS            64
#define FCACCEPT_V2_BASICS              128
#define FCACCEPT_V2_BOOKMARKS           256
#define FCACCEPT_V2_TOPFRIENDS          512
#define FCACCEPT_V2_CLUBMEMBERS         1024

// Bitmask values for various reasons why a private, trueprivate, voyeur, or group session
// was either denied, failed to start, or a request was blocked.
#define FCNOSESS_NONE                   0
#define FCNOSESS_PVT                    1
#define FCNOSESS_GRP                    2
#define FCNOSESS_TRUEPVT                4
#define FCNOSESS_TOKEN_MIN              8
#define FCNOSESS_PLATFORM               16
#define FCNOSESS_VIDEOSERVER            32
#define FCNOSESS_INVALID_STATE          64
#define FCNOSESS_MODEL_SETTINGS         128
#define FCNOSESS_CLIENT_ERROR           256

// bitmask values for dwModelFlags in BROADCASTPROFILE packets
#define FCMODEL_NONE                    0
#define FCMODEL_NOGROUP                 1
#define FCMODEL_FEATURE1                2
#define FCMODEL_FEATURE2                4
#define FCMODEL_FEATURE3                8
#define FCMODEL_FEATURE4                16
#define FCMODEL_FEATURE5                32

// User Created Rooms options (UCR)
#define FCUCR_VM_LOUNGE                 0           // video_mode 'lounge' (default video mode style for UCR)
#define FCUCR_VM_MYWEBCAM               1           // video_mode 'my webcam' - cam2cam feed of creator of room

#define FCUCR_CREATOR                   0
#define FCUCR_FRIENDS                   1
#define FCUCR_MODELS                    2
#define FCUCR_PREMIUMS                  4
#define FCUCR_BASICS                    8
#define FCUCR_ALL                       (FCUCR_FRIENDS | FCUCR_MODELS | FCUCR_PREMIUMS | FCUCR_BASICS)

// Sent as arg1 for FCTYPE_SERVERREFRESH event to server, also the dwType for a FCMSG_Q in HK_LOG_EVENT
// hooks where qwArg is FCTYPE_EXTDATA
#define FCUPDATE_NONE                   0
#define FCUPDATE_MISSMFC                1           // Update online ranked models from miss_mfc table
#define FCUPDATE_NEWTIP                 2           // New tip posted, process tips
#define FCUPDATE_REGION_SAFE            3           // Update to 'chat_region_overrides' table
#define FCUPDATE_CAMSCORE               4           // Update to camscore for model in next argument
#define FCUPDATE_ROOMFILTER             5           // Update to Chat Room Filters for model in argument
#define FCUPDATE_CLUBMEMBERSHIP         6           // Update to list of club memberships for a model
#define FCUPDATE_CLUB                   7           // Update to list of clubs for a model
#define FCUPDATE_CLUBCACHE              8           // Update the cache stored in redis (to edge from master)
#define FCUPDATE_SHAREDATA              9           // General msg from sahre (JSON data for rpc proxy)


// bitmask values for dwUserFlags in user hash packets (was nChatEffects)
enum FcOpt
{
    FCOPT_NONE                      = (  ZERO64   ),    // Normal (no options)
    FCOPT_BOLD                      = (ONE64 <<  0),    // Chat effect: user's text is bold
    FCOPT_ITALICS                   = (ONE64 <<  1),    // Chat effect: user's text is in italics
    FCOPT_REMOTEPVT                 = (ONE64 <<  2),    // Model effect: this model is actually in a Remote Pvt
    FCOPT_TRUEPVT                   = (ONE64 <<  3),    // Model effect: this model is actually in a TruePvt
    FCOPT_CAM2CAM                   = (ONE64 <<  4),    // User effect: user also has their cam broadcasting
    FCOPT_RGNBLOCK                  = (ONE64 <<  5),    // Model effect: maintains a region ban list for who sees their status
                                                        // from model_settings 'block_chat'
    FCOPT_TOKENAPPROX               = (ONE64 <<  6),    // User effect: Approximate token count for models
    FCOPT_TOKENHIDE                 = (ONE64 <<  7),    // User effect: Hide token count for models
    FCOPT_RPAPPROX                  = (ONE64 <<  8),    // User effect: Approximate RP for models
    FCOPT_RPHIDE                    = (ONE64 <<  9),    // User effect: Hide RP for models
    FCOPT_HDVIDEO                   = (ONE64 << 10),    // Model effect: Video associated with broadcaster is an HD feed
    FCOPT_MODELSW                   = (ONE64 << 11),    // Model effect: Model is logged in with model software (modelweb,modelexe)
    FCOPT_GUESTMUTE                 = (ONE64 << 12),    // Model effect: Model has guests muted in channel
    FCOPT_BASICMUTE                 = (ONE64 << 13),    // Model effect: Model has basics muted in channel
    FCOPT_SMALLCAPS                 = (ONE64 << 14),    // Chat effect: user's text is in smallcaps css style
    FCOPT_XMPP                      = (ONE64 << 15),    // Client effect: user is logged in via XMPP Proxy
    FCOPT_WHITEBOARD1               = (ONE64 << 16),    // Model effect: Model has public whiteboard turned on
    FCOPT_WHITEBOARD2               = (ONE64 << 17),    // Model effect: Model has private or group whiteboard turned on


    // Model effect: Model has attached to an OBS (or other type) video feed stream. This
    // bit is turned on when model attaches to a feed, and turned off when model
    // unattaches from feed. Unattaches may happen either manually by user action
    // (clicking the 'Attach to feed' in ModelExe) or automatically, as the side-effect
    // of another event (model's pvt session closed because user ran out of tokens, or
    // model session which attached the feed disconnects from chat, etc.)
    //
    FCOPT_ATTACHED                  = (ONE64 << 18),

    // Set in model flags for webrtc/obs-webrtc version 1 video. FCOPT_WEBRTCV1 indicates
    // a legacy stream name syntax (eg. 'mfc_100000266'). FCOPT_OBSRTCV1 indicates the
    // model's stream name syntax includes a phase code (eg. 'mfc_a_100000266').
    //
    FCOPT_WEBRTCV1                  = (ONE64 << 19),
    FCOPT_OBSRTCV1                  = (ONE64 << 20),

    // Special Notes: The FCOPT_OBSRTCV2 flag indicates an OBS formatted stream is being
    // emulated for the model, but *not* advertised as OBS to any other clients. This was
    // added to support use of the OBS/nginx stream format messages when ModelWeb and FCS
    // chat servers interact (which all require the stream to have a phase code embedded)
    // when OBS is streaming WebRTC format video but to a wowza server that isn't
    // configured to accept streams with phase codes embedded.
    //
    FCOPT_OBSRTCV2                  = (ONE64 << 21),

    // FCOPT_SIDEKICK set when an active sidekick instance has attached or bound itself
    // to this model session.
    //
    FCOPT_SIDEKICK                  = (ONE64 << 22),

    // FCOPT_SPLITCAM set when an active splitcam instance has attached or bound itself
    // to this model session. NOT CURRENTLY IN USE OR SUPPORTED YET. Splitcam association
    // will require  periodic active checkins by splitcam to ensure the plugin is still
    // running and active for this model session.
    //
    FCOPT_SPLITCAM                  = (ONE64 << 23),

    // set when a guest connection has authenticated as an agent for a specific model.
    // if FCOPT_AGENT is set in m_user.nChatOptions, then m_user.nLastLogin is used to
    // store the model id that the agent client is bound to. This data is not copied
    // to edgeservers or sent out to clients since guests normally have no usable data
    // stored in UserData::m_user. Option only visible on master.
    //
    FCOPT_AGENT                     = (ONE64 << 24),
};



// Bitmask for 'user_opts' columns in several tables (token_sessions, ots, channel_events, channel_log etc) describes
// properties of user (or model) at time of event.
enum FcUserOpt
{
    FCUOPT_EMPTY                    = (  ZERO64   ),        // No attributes set

    FCUOPT_PLATFORM_MFC             = (ONE64 <<  0),        // user is logged in through MFC
    FCUOPT_PLATFORM_CAMYOU          = (ONE64 <<  1),        // user is logged in through Camyou
    FCUOPT_PLATFORM_OFFLINE         = (ONE64 <<  2),        // user is logged off
    FCUOPT_PLATFORM_XSDEFAULT       = (ONE64 <<  3),        // Bitmask value for cross-site platform default.
                                                            // Used in dwPlatforms value mask in UserExtensions
                                                            // to designate the UEOPT defaults to any option
                                                            // set, even from another platform. Only applies to
                                                            // options that are not set specifically on the
                                                            // client's current platform however.
    //
    // Other options that can be turned on to describe
    // user at time of event or action being logged
    //
    FCUOPT_RESERVED_05              = (ONE64 <<  4),        // Reserved Option
    FCUOPT_RESERVED_06              = (ONE64 <<  5),        // Reserved Option
    FCUOPT_RESERVED_07              = (ONE64 <<  6),        // Reserved Option

    FCUOPT_LOG_XMESG                = (ONE64 <<  7),        // Designates sender of message is XMESG json,
                                                            // used in useropts of chat logs

    FCUOPT_RESERVED_09              = (ONE64 <<  8),        // Reserved Option
    FCUOPT_RESERVED_10              = (ONE64 <<  9),        // Reserved Option
    FCUOPT_RESERVED_11              = (ONE64 << 10),        // Reserved Option
    FCUOPT_RESERVED_12              = (ONE64 << 11),        // Reserved Option
    FCUOPT_RESERVED_13              = (ONE64 << 12),        // Reserved Option
    FCUOPT_RESERVED_14              = (ONE64 << 13),        // Reserved Option
    FCUOPT_RESERVED_15              = (ONE64 << 14),        // Reserved Option
    FCUOPT_RESERVED_16              = (ONE64 << 15),        // Reserved Option
    FCUOPT_RESERVED_17              = (ONE64 << 16),        // Reserved Option

    FCUOPT_FLAG_HDVIDEO             = (ONE64 << 17),        // Mirrors FCOPT_HDVIDEO: publishing 'HD' stream
    FCUOPT_FLAG_MODELSW             = (ONE64 << 18),        // Mirrors FCOPT_MODELSW: logged in with modelweb or modelexe
    FCUOPT_FLAG_XMPP                = (ONE64 << 19),        // Mirrors FCOPT_XMPP: logged in as XMPP client
    FCUOPT_FLAG_WHITEBOARD1         = (ONE64 << 20),        // Mirrors FCOPT_WHITEBOARD1: primary whiteboard active
    FCUOPT_FLAG_WHITEBOARD2         = (ONE64 << 21),        // Mirrors FCOPT_WHITEBOARD2: secondary whiteboard active
    FCUOPT_FLAG_ATTACHED            = (ONE64 << 22),        // Mirrors FCOPT_ATTACHED: model is attached to an OBS stream
    FCUOPT_FLAG_WEBRTCV1            = (ONE64 << 23),        // Mirrors FCOPT_WEBRTCV1: model is broadcasting via WebRTC

    //---/ FCUOPT_FLAG_NG_EDGE /--------------------------------------------------------------------
    // Indicates a server_config/transcode_servers row is an edge chat node designated to
    // handle RTMP notify callbacks from rtmp module in nginx.
    //
    FCUOPT_FLAG_NG_EDGE             = (ONE64 << 24),

    //---/ FCUOPT_FLAG_NG_ADDR_ORIGIN /------------------------------------------------------------
    // When the 'opt' column of a server_config or transcode_servers row has this bit set, it
    // indicates the nginx server is an Address-catching default or fallthrough origin server.
    // These servers are used to accept streams being published by models from an unrecognized
    // or unknown IP address. The server will save the address in a table associating the IP with
    // the model from their CtxToken or other auth details, force a IP table rebuild for the
    // HA-Proxy balancers routing rtmp stream publishing, and then disconnect the client. When the
    // client reconnects, HA-Proxy should recognize the IP and map them to a different RTMP server.
    //
    FCUOPT_FLAG_NG_ADDR_ORIGIN      = (ONE64 << 25),


    FCUOPT_FLAG_OBSRTCV1            = (ONE64 << 26),    // FCOPT_OBSRTCV1: WebRTC via OBS w/phase
    FCUOPT_FLAG_OBSRTCV2            = (ONE64 << 27),    // FCOPT_OBSRTCV2: WebRTC via OBS w/o phase
    FCUOPT_FLAG_SIDEKICK            = (ONE64 << 28),    // FCOPT_SIDEKICK: Running Sidekick OBS plugin
    FCUOPT_FLAG_SPLITCAM            = (ONE64 << 29),    // FCOPT_SPLITCAM: Running Splitcam broadcast app
};

//---/ NotifyEvType /-------------------------------------------------------------------------
//
// Used as arg1 for FCTYPE_NOTIFY messages, and stored in payload of FCTYPE_NOTIFY json object
// as well in the evType key (with evType's value being an integer).  Each FCTYPE_NOTIFY
// message should have both evType key (integer of NotifyEvType) and subev (usually a string,
// but can depend on the evType)
//
enum NotifyEvType
{
    NEV_NULL        = 0,        // Emtpy/Null event type

    // Clients must opt-in with op:subscribe for these events
    NEV_WBFLASH,                // Whiteboard flash event type
    NEV_WBACL,                  // Whiteboard ACL defined or changed, rebuilds the access list
                                // of read/write access to a channel's whiteboard


    //---/ NEV_PUBLISH /----------------------------------------------------------------------
    // Publish event type.  May describe a stream that has just been published (onPublish),
    // that has just ended (onPublishDone), that is continuing (onUpdatePublish), or that
    // was rejected by the server for some reason. Other notifications describing a stream's
    // current video state may also be delivered using the NEV_PUBLISH event type in a
    // FCTYPE_NOTIFY message.
    //
    //
    // When a model publishes a stream with OBS (or another encoder capable of publishing to
    // nginx video servers), various FCTYPE_NOTIFY messages are delivered to each model login
    // instance without the model specifically opting-in or subscribing to this event type.
    // Messages describing stream states with this event type are processed by first looking
    // in the JSON payload of notify message for a key named 'subev' with one of these
    // values:
    //
    // started      stream starts,
    //
    // softdc       soft disconnect, or unexpected disconnect of encoder
    //
    // resume       reconnect & resume publish of a stream which recently soft disconnected
    //
    // timeout      the grace period has ended for a soft disconnected stream without the
    //              model reconnecting and resuming the stream.
    //
    // update       the stream updated.  Update to stream is not a major update such as
    //              state change from start to stopped, rather updating values such as the
    //              expiration timeout has been set, extended, or unset, or stream tags
    //              have updated, or some other meta data (bitrate, resolution, etc) has
    //              been filled out or changed.
    //
    // ended        hard disconnect, or intentionally ended stream by model (no grace period
    //              issued, meaning no resume of feed would be possible)
    //
    // The stream's state and description data is a JSON serialized VIDEO_FEEDSTATE structure,
    // and is an object attached to the 'vfs' key in the root of the notify object.
    //
    //
    NEV_PUBLISH,

    //---/ NEV_PUBLISH Notes /----------------------------------------------------------------
    // The NEV_PUBLISH type notification for a FCTYPE_NOTIFY message differs from the
    // FCTYPE_PUBLISH messages a model may receive in that it represents a lower level,
    // transactional update or state change to the current collection of streams associated
    // with a model. In many cases this means the model will be notified of stream state
    // changes through both message types, however some state changes for a stream may
    // ONLY be sent to a model in the form of a FCTYPE_NOTIFY message, depending on the
    // type of event or state change.
    //
    // If, for example, a model intentionally ends a stream that was being actively
    // published, the server would send that model's logins a FCTYPE_PUBLISH
    // message with a list of any remaining or active streams, and the server would also
    // send a FCTYPE_NOTIFY message of NEV_PUBLISH event type describing the shutdown
    // of the stream.
    //
    // If the encoder disconnected unintentionally, however, the server treats the
    // stream ending as a soft disconnect. In this case no FCTYPE_PUBLISH will be
    // immediately issued, but a FCTYPE_NOTIFY / NEV_PUBLISH message would.  The notify
    // message with a subev value of softdc would be sent so that the model software
    // could show a notification that OBS had disconnected, and may need to reconnect
    // if it wasnt configured to do so automatically.  If the model's encoder didnt
    // reconnect within the grace period, a FCTYPE_PUBLISH message would be issued,
    // one that showed the stream had been dropped.
    //
    // NEV_PUBLISH notifications should be used to keep the model software aware
    // of passive changes such as notice of reconnecting attempts.
    //-----------------------------------------------------------------/ NEV_PUBLISH Notes /---


    //---/ NEV_CLUBCOUNT /----------------------------------------------------------------------
    // Club online count change event type.  Sent to models who have clubs and clubgroups
    // periodically with updated counts on how many club members or club group (aggregate)
    // members are online, and how many there are total.
    //
    // A key named 'data' and it's object value store a series of club id and clubgroup ids
    // owned by the model recieving the notification, with the keys being string versions
    // of the id and values are an array of two elements, the online and total count for
    // that club or clubgroup.  Clubgroups can be identified by having an id >2billion.
    //
    NEV_CLUBCOUNT,



    // Generic events, may not require client to opt-in depending on how cgi sends events (to uid/sid or group?)
    // Any value >= NEV_CUSTOM is considered NEV_CUSTOM type.  Each cgi app can pick it's own id as the group type.
    NEV_CUSTOM      = 100000,
};

#define FCBAN_NONE                      0           // No ban in effect
#define FCBAN_TEMP                      1           // Temporary ban (4 hour interval max) in effect
#define FCBAN_60DAY                     2           // 60 Day ban in effect
#define FCBAN_LIFE                      3           // Lifetime ban in effect

#define FCCHAN_EVENT_NONE               0           // Null/Empty channel_event log entry
#define FCCHAN_EVENT_CLEARCHAT          1           // this channel_event log entry means a ban happened and clear chat revised history. Hide
                                                    // chat from the user or guest with matching IP from date_sent time and BACK event_arg1 minutes.
                                                    // i.e. channel_id 100000100, date_sent of 2016-08-25 00:41:00, event_arg1: 30, user_id: 100.
                                                    // Must hide any chatlog messages (or tips) from user_id 100 that was sent to channel_id:100000100
                                                    // from 2016-08-25 00:11:00 to  2016-08-25 00:41:00.
#define FCCHAN_EVENT_MUTE               2           // [NOT IMPLEMENTED] a channel mute (event_arg1 dictates guest, basics?)
#define FCCHAN_EVENT_TOPIC              3           // [NOT IMPLEMENTED] topic set?
#define FCCHAN_EVENT_COUNTDOWN          4           // [NOT IMPLEMENTED] tip countdown?
#define FCCHAN_EVENT_KICK               5           // [NOT IMPLEMENTED] user was kicked?
#define FCCHAN_EVENT_WHITEBOARD_ON      6           // whiteboard turned on
#define FCCHAN_EVENT_WHITEBOARD_OFF     7           // whiteboard turned off
#define FCCHAN_EVENT_RESERVED_008       8           // [NOT IMPLEMENTED] to be changed as events
#define FCCHAN_EVENT_RESERVED_009       9           // [NOT IMPLEMENTED] are added
#define FCCHAN_EVENT_RESERVED_010       10          // [NOT IMPLEMENTED]
#define FCCHAN_EVENT_RESERVED_011       11          // [NOT IMPLEMENTED]
#define FCCHAN_EVENT_RESERVED_012       12          // [NOT IMPLEMENTED]
#define FCCHAN_EVENT_RESERVED_013       13          // [NOT IMPLEMENTED]
#define FCCHAN_EVENT_RESERVED_014       14          // [NOT IMPLEMENTED]
#define FCCHAN_EVENT_RESERVED_015       15          // [NOT IMPLEMENTED]
#define FCCHAN_EVENT_RESERVED_016       16          // [NOT IMPLEMENTED]
#define FCCHAN_EVENT_RESERVED_017       17          // [NOT IMPLEMENTED]
#define FCCHAN_EVENT_RESERVED_018       18          // [NOT IMPLEMENTED]
#define FCCHAN_EVENT_RESERVED_019       19          // [NOT IMPLEMENTED]




#define FCNEWSOPT_NONE                  0           // for newsfeedDB related options
#define FCNEWSOPT_IN_CHAN               1           // send news item to a channel window on client
#define FCNEWSOPT_IN_PM                 2           // send news item to a PM window on client
#define FCNEWSOPT_AUTOFRIENDS_OFF       4           // don't auto-follow all friends
#define FCNEWSOPT_IN_CHAN_NOPVT         8           // don't send item to a channel window if its in private
#define FCNEWSOPT_IN_CHAN_NOGRP         16          // don't send item to a channel window if its in group

// Values supported in client for user_settings.online_model_order
#define MODELORDER_NONE                 0           // default sort options
#define MODELORDER_PVT                  1           // pvt up top
#define MODELORDER_TRUEPVT              2           // truepvt up top
#define MODELORDER_GRP                  4           // group up top

#pragma pack(push, 1)

typedef struct _FCMSG
{
    uint32_t dwMagic;                   // must be PROTOCOL_MAGIC
    uint32_t dwType;                    // See FCTYPE_ defines
    uint32_t dwFrom;
    uint32_t dwTo;
    uint32_t dwArg1;
    uint32_t dwArg2;
    uint32_t dwMsgLen;
} FCMSG;

// Same as FCMSG, but has room for pszMsg at the end. Used by streaming msg queue
typedef struct
{
    uint32_t dwMagic;                   // must be PROTOCOL_MAGIC
    uint32_t dwType;                    // See FCTYPE_ defines
    uint32_t dwFrom;
    uint32_t dwTo;
    uint32_t dwArg1;
    uint32_t dwArg2;
    uint32_t dwMsgLen;
    char*    pchMsg;
} FCMSG_Q;

enum FcLoginType
{
    FCLOGIN_LEGACY              = 0,    // Client login with user/pass text credentials, forwarded from edgechat
    FCLOGIN_AUTH                = 40,   // Authserver login on FcsAuth prototocol
    FCLOGIN_WORKER              = 41,   // Downstream Worker login (similar to EdgeChat login, but no clients come through worker)
    FCLOGIN_EDGECHAT            = 42,   // Downstream EdgeChat server login
    FCLOGIN_PRELOG              = 43,   // EdgeChat relayed login data pre-authenticated with FCPRELOG payload
};

// FCJIP - storage space for either a IPv4 or IPv6 address.  Abstracted to it's own structure
//         for easier use in containers
struct FCJIP
{
    union
    {
        struct in_addr  v4;             // IPv4 addr in 64bit int
        struct in6_addr v6;             // IPv6 Addr
    };
};

#pragma pack(pop)

#define MAX_USER_SETTINGS       64
#define MAX_MODELGROUPS         3
#define MAX_ROOMDATA            160     // send at most this number of room updates in a single FCTYPE_ROOMDATA update

// VideoStates:
#define IS_TXSTATE(dw)              (dw < FCVIDEO_RX_IDLE)
#define IS_RXSTATE(dw)              (dw >= FCVIDEO_RX_IDLE)
#define IS_BROADCASTING(dw)         (dw == FCVIDEO_TX_IDLE || dw == FCVIDEO_TX_PVT || dw == FCVIDEO_TX_GRP)
#define IN_TX_SESSION(dw)           (dw == FCVIDEO_TX_PVT || dw == FCVIDEO_TX_GRP)
#define IN_RX_SESSION(dw)           (dw == FCVIDEO_RX_PVT || dw == FCVIDEO_RX_VOY || dw == FCVIDEO_RX_GRP)
#define IS_IDLE(dw)                 (dw == FCVIDEO_RX_IDLE || dw == FCVIDEO_TX_IDLE || dw == FCVIDEO_TX_AWAY)
#define IS_SESSION(dw)              (IN_TX_SESSION(dw) || IN_RX_SESSION(dw))

#define MAX_RANKED_MODELS           1000        // Max number of miss_mfc ranks to cache from miss_mfc table

// Constants used as keys across multiple extensions to access values stored in a UserData object's
// m_mExtStrVals, m_mExtNumVals, or m_mGenStorV maps
//
// UserData::m_mExtStrVals key for Topic Template (before any translation)
// for models. 2 is ext_roomdata's extension id.
#define UDEXT_TOPIC                 (MAKE_I32(2, FCTYPE_SETWELCOME))

