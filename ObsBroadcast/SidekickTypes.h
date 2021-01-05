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

#ifndef SIDEKICK_TYPES_H_
#define SIDEKICK_TYPES_H_

typedef void (*SIDEKICK_TIMER_CB)(unsigned int interval, unsigned int elapsed);

// State of sidekick, used to display current status in sidekick log window
// as well as govern logic for when stream may start and how heartbeat API calls
// are made.
//
enum SidekickActiveState
{
    SkUninitialized         = 0,    // Unknown/uninitialized state
    SkUnknownProfile        = 1,    // Profile is not setup for MFC WebRTC, sidekick disabled.
    SkNoCredentials         = 2,    // No MFC UserId or StreamKey credentials, requires login
    SkInvalidCredentials    = 3,    // Credentials expired or invalid
    SkNoModelwebSession     = 4,    // No Modelweb session detected.
    SkStreamStarting        = 11,   // Initiated publish operation...
    SkStreamStarted         = 12,   // Successfully publishing stream to WebRTC
    SkStreamStopping        = 13,   // Initiated unpublish/stop streaming operation..
    SkStreamStopped         = 20,   // Stream stopped but ready to start, waiting on model to click 'Start Streaming'
};

enum SidekickEventType
{
    SkNull                  = 0,    // null/empty event type
    SkLog,                          // Generic event type for logging request by worker thread
    SkSessionId,                    // session id updated
    SkStreamKey,                    // stream key updated
    SkServerUrl,                    // server URL updated
    SkUserId,                       // user id updated
    SkUnlink,                       // unlink event (agentSvc reports invalid token)
    SkModelState,                   // new activeState for model (arg1 changed, arg2 oldState, arg3 newState)
    SkRoomEv,                       // room event received
    SkReadProfile,                  // request UI thread to exec onObsProfileChanged() to reprocess current profile in UI dlg
    SkSetWebRtc                     // set current Sidekick supported profile to WebRTC (if arg1 is 1) or RTMP (if arg1 is 0)
};

typedef struct
{
    unsigned int interval;          // callback interval, in milliseconds. 1000 = 1second
    unsigned int elapsed;           // milliseconds elapsed since last callback made.
    SIDEKICK_TIMER_CB pFunc;        // pointer to callback function
} SIDEKICK_TIMER_EV;

typedef struct
{
    SidekickEventType   ev;         // event type: ctx updates, api error response, api timeout, or status/log message
    struct timeval      tvStamp;    // generated @ time for event

    uint32_t            dwArg1;     // dword args 1
    uint32_t            dwArg2;     // dword args 2
    uint32_t            dwArg3;     // dword args 3
    uint32_t            dwArg4;     // dword args 4

    std::string         sArg1;      // string arg 1
    std::string         sArg2;      // string arg 2
} SIDEKICK_UI_EV;

#endif  // SIDEKICK_TYPES_H_