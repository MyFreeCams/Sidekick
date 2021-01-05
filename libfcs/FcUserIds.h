#pragma once

// Hardcoded users.id values for specific test or role account users

//---/ model accounts /-----------------------------------------------------------------------------
//
#define FCUSER_AARONCAM         100         // @jshidden AaronCam model
#define FCUSER_TESTCAM1         469518      // TestCam1 model
#define FCUSER_TESTCAM2         469516      // TestCam2 model
#define FCUSER_TESTCAM3         469517      // TestCam3 model
#define FCUSER_TESTCAM4         266         // @jshidden TestCam4
#define FCUSER_TESTCAM4X        267         // @jshidden TestCam4x
#define FCUSER_TESTCAM5         268         // @jshidden TestCam5
#define FCUSER_TESTCAM6         449         // @jshidden TestCam6
#define FCUSER_TESTCAM7         70001       // @jshidden TestCam7
#define FCUSER_TESTCAM8         70002       // @jshidden TestCam8
#define FCUSER_TESTCAM9         70003       // @jshidden TestCam9

//---/ user accounts /------------------------------------------------------------------------------
//
#define FCUSER_XMFC1            127465      // @jshidden xmfc1 user (basic)
#define FCUSER_XMFC2            127466      // @jshidden xmfc2 user (premium)
#define FCUSER_XMFC3            127467      // @jshidden xmfc3 user (premium)
#define FCUSER_XMFC4            164056      // @jshidden xmfc4 user (premium)
#define FCUSER_MFCUSER          36948       // @jshidden mfcuser user (premium)
#define FCUSER_MFCGUY           16558       // @jshidden mfcguy


//---/ fcs-<role> accounts /------------------------------------------------------------------------
//
#define FCUSER_AUTH             270         // @jshidden uid for fcs-auth
#define FCUSER_TRANSCODE        271         // @jshidden uid for fcs-transcode
#define FCUSER_TESTPLAYER       272         // @jshidden uid for fcs-testplayer
#define FCUSER_SIDEKICK         273         // @jshidden uid for fcs-sidekick
#define FCUSER_RESERVED4        274         // @jshidden uid for fcs-reserved4
#define FCUSER_RESERVED5        275         // @jshidden uid for fcs-reserved5
#define FCUSER_RESERVED6        276         // @jshidden uid for fcs-reserved6
#define FCUSER_RESERVED7        277         // @jshidden uid for fcs-reserved7
#define FCUSER_RESERVED8        278         // @jshidden uid for fcs-reserved8
#define FCUSER_RESERVED9        279         // @jshidden uid for fcs-reserved9
#define FCUSER_RESERVED10       280         // @jshidden uid for fcs-reserved10


//---/ News/Share role accounts /-------------------------------------------------------------------
//
#define FCUSER_MFCNEWS          481462      // UID for "MyFreeCams_News", everyone on mfc follows
#define FCUSER_CAMNEWS          481464      // UID for "CamYou_News", everyone on camyou follows
#define FCUSER_MFCSHARE         1535156     // UID for "MFCShare" - MFC's Share system account
#define FCUSER_CAMSHARE         1535157     // UID for "CamShare" - CamYou's Share system account


//---/ Lounge role accounts /-----------------------------------------------------------------------
//
#define FCUSER_LOUNGE           486121      // Lounge
#define FCUSER_LOUNGE1K         486123      // Lounge1000
#define FCUSER_LOUNGE10K        486124      // Lounge10000
#define FCUSER_CAM_LOUNGE       486128      // CamYou Lounge
#define FCUSER_CAM_LOUNGE_1K    486129      // @jshidden CamYou Lounge1000 (Unused)
#define FCUSER_CAM_LOUNGE_10K   486130      // @jshidden CamYou Lounge10000 (Unused)


//---/ Test uid macros /----------------------------------------------------------------------------
// Test if uid is one of a few specific model accounts
//
#ifndef TESTCAM_MODEL           
#define TESTCAM_MODEL(dwUserId) ( ( dwUserId == FCUSER_TESTCAM2         \
                                 || dwUserId == FCUSER_TESTCAM3         \
                                 || dwUserId == FCUSER_AARONCAM         \
                                 || dwUserId == FCUSER_TESTCAM1         \
                                 || dwUserId == FCUSER_TESTCAM7         \
                                 || dwUserId == FCUSER_TESTCAM4       ) )
#endif
//
// Test if uid is one of a few specific user accounts
//
#ifndef TESTCAM_USER
#define TESTCAM_USER(dwUserId) ( ( dwUserId == FCUSER_MFCGUY            \
                                || dwUserId == FCUSER_MFCUSER           \
                                || dwUserId == FCUSER_XMFC1             \
                                || dwUserId == FCUSER_XMFC2             \
                                || dwUserId == FCUSER_XMFC3             \
                                || dwUserId == FCUSER_XMFC4           ) )
#endif
//
//----------------------------------------------------------------------------/ Test uid macros /---

