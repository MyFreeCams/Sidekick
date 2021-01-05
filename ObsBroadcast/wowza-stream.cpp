/*
 * Copyright (c) 2013-2020 MFCXY, Inc. <mfcxy@mfcxy.com>
 * Copyright (c) 2020 CoSMo - Dr Alex Gouaillard <contact@cosmosoftware.io>
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
#include "wowza-stream.h"
#include "WebRTCStream.h"

#include <libobs/obs-module.h>

// silence warnings in MSVC
#ifdef _WIN32
#pragma warning (disable: 4003)  // enough actual parameters for macro
#pragma warning (disable: 4005)  // macro redefinition
#pragma warning (disable: 4244)
#pragma warning (disable: 4245)
#pragma warning (disable: 4840)  // non-portable use of class 'type' as an argument to a variadic function
#endif

#define info(format, ...) blog(300, "[wowza output] " format, ##__VA_ARGS__)
#define trace() info("%s", __func__)

#define OPT_DROP_THRESHOLD          "drop_threshold_ms"
#define OPT_PFRAME_DROP_THRESHOLD   "pframe_drop_threshold_ms"
#define OPT_MAX_SHUTDOWN_TIME_SEC   "max_shutdown_time_sec"
#define OPT_BIND_IP                 "bind_ip"
#define OPT_NEWSOCKETLOOP_ENABLED   "new_socket_loop_enabled"
#define OPT_LOWLATENCY_ENABLED      "low_latency_mode_enabled"

extern "C" const char* wowza_stream_getname(void* unused)
{
    UNUSED_PARAMETER(unused);
    return obs_module_text("WOWZAStream");
}


extern "C" void* wowza_stream_create(obs_data_t* settings, obs_output_t* output)
{
    trace();
    UNUSED_PARAMETER(settings);
    WebRTCStream* stream = new WebRTCStream(output);    // Create new stream
    stream->AddRef();                                   // So it won't be deleted automatically
    return (void*)stream;
}


extern "C" bool wowza_stream_start(void* data)
{
    trace();
    bool retVal = false;
    WebRTCStream* stream = (WebRTCStream*)data;
    stream->AddRef();
    retVal = stream->Start();

    return retVal;
}


extern "C" void wowza_stream_stop(void* data, uint64_t ts)
{
    trace();
    UNUSED_PARAMETER(ts);
    WebRTCStream* stream = (WebRTCStream*)data;
    stream->Stop(true);
    stream->Release();                                  // Remove ref and let it self destroy
}


extern "C" void wowza_stream_destroy(void* data)
{
    trace();
    WebRTCStream* stream = (WebRTCStream*)data;
    stream->Stop(false);
    stream->Release();                                  // Remove ref and let it self destroy
}


extern "C" void wowza_receive_video(void* data, video_data* frame)
{
    WebRTCStream* stream = (WebRTCStream*)data;
    stream->onVideoFrame(frame);
}


extern "C" void wowza_receive_audio(void* data, audio_data* frame)
{
    WebRTCStream* stream = (WebRTCStream*)data;
    stream->onAudioFrame(frame);
}


extern "C" void wowza_receive_multitrack_audio(void* data, size_t idx, audio_data* frame)
{
    UNUSED_PARAMETER(idx);
    WebRTCStream* stream = (WebRTCStream*)data;
    stream->onAudioFrame(frame);
}


extern "C" void wowza_stream_defaults(obs_data_t* defaults)
{
    obs_data_set_default_int(defaults,      OPT_DROP_THRESHOLD,         700);
    obs_data_set_default_int(defaults,      OPT_PFRAME_DROP_THRESHOLD,  900);
    obs_data_set_default_int(defaults,      OPT_MAX_SHUTDOWN_TIME_SEC,  30);
    obs_data_set_default_string(defaults,   OPT_BIND_IP,                "default");
    obs_data_set_default_bool(defaults,     OPT_NEWSOCKETLOOP_ENABLED,  false);
    obs_data_set_default_bool(defaults,     OPT_LOWLATENCY_ENABLED,     false);
}


extern "C" obs_properties_t* wowza_stream_properties(void* unused)
{
    UNUSED_PARAMETER(unused);
    obs_properties_t* props = obs_properties_create();
    obs_properties_add_int(props, OPT_DROP_THRESHOLD,
                           obs_module_text("WOWZAStream.DropThreshold"),
                           200, 10000, 100);
    obs_properties_add_bool(props, OPT_NEWSOCKETLOOP_ENABLED,
                            obs_module_text("WOWZAStream.NewSocketLoop"));
    obs_properties_add_bool(props, OPT_LOWLATENCY_ENABLED,
                            obs_module_text("WOWZAStream.LowLatencyMode"));
    return props;
}


extern "C" uint64_t wowza_stream_total_bytes_sent(void* data)
{
    WebRTCStream* stream = (WebRTCStream*)data;
    return stream->totalBytesSent();
}


extern "C" int wowza_stream_dropped_frames(void* data)
{
    WebRTCStream* stream = (WebRTCStream*)data;

    // OBS calls |wowza_stream_dropped_frames| every 1 second vs every 2 seconds for
    // |wowza_stream_total_bytes_sent|. Calling GetStats here results in more
    // consistent bitrate readings.
    stream->GetStats();

    // Rough estimation of dropped frames
    return (int)std::max(stream->framesDropped(), std::max(stream->pliReceived(), stream->nackReceived()));
}


// Changes the color of the stream status square in the bottom right corner of OBS
// based on deviations from input fps & resolution.
extern "C" float wowza_stream_congestion(void* data)
{
    WebRTCStream* stream = (WebRTCStream*)data;
    const uint32_t inResolution = stream->width() * stream->height();
    const uint32_t outResolution = stream->currentWidth() * stream->currentHeight();
    const double inFps = obs_get_active_fps();
    const double outFps = stream->currentFps();
    const double fpsPercentile = (outFps + 1) / (inFps + 1);  // prevent floating-point division by 0
    const double resolutionPercentile = sqrt((double)outResolution) / sqrt((double)inResolution);
    const double congestion = 1.0 - fpsPercentile / 2 - resolutionPercentile / 2;
    return (float)congestion;
}


extern "C"
{
#ifdef _WIN32 // to avoid MSVC compilation error
    obs_output_info wowza_output_info =
    {
        "mfc_wowza_output",                 //id
        OBS_OUTPUT_AV | OBS_OUTPUT_SERVICE, //flags
        wowza_stream_getname,               //get_name
        wowza_stream_create,                //create
        wowza_stream_destroy,               //destroy
        wowza_stream_start,                 //start
        wowza_stream_stop,                  //stop
        wowza_receive_video,                //raw_video
        wowza_receive_audio,                //raw_audio (single-track)
        nullptr,                            //encoded_packet
        nullptr,                            //update
        wowza_stream_defaults,              //get_defaults
        wowza_stream_properties,            //get_properties
        nullptr,                            //unused1 (formerly pause)
        wowza_stream_total_bytes_sent,      //get_total_bytes
        wowza_stream_dropped_frames,        //get_dropped_frames
        nullptr,                            //type_data
        nullptr,                            //free_type_data
        wowza_stream_congestion,            //get_congestion
        nullptr,                            //get_connect_time_ms
        "h264",                             //encoded_video_codecs
        "opus",                             //encoded_audio_codecs
        nullptr,                            //raw_audio2 (multi-track)
    };
#else
    obs_output_info wowza_output_info =
    {
        .id                   = "mfc_wowza_output",
        .flags                = OBS_OUTPUT_AV | OBS_OUTPUT_SERVICE,
        .get_name             = wowza_stream_getname,
        .create               = wowza_stream_create,
        .destroy              = wowza_stream_destroy,
        .start                = wowza_stream_start,
        .stop                 = wowza_stream_stop,
        .raw_video            = wowza_receive_video,
        .raw_audio            = wowza_receive_audio, // for single-track
        .encoded_packet       = nullptr,
        .update               = nullptr,
        .get_defaults         = wowza_stream_defaults,
        .get_properties       = wowza_stream_properties,
        .unused1              = nullptr,
        .get_total_bytes      = wowza_stream_total_bytes_sent,
        .get_dropped_frames   = wowza_stream_dropped_frames,
        .type_data            = nullptr,
        .free_type_data       = nullptr,
        .get_congestion       = wowza_stream_congestion,
        .get_connect_time_ms  = nullptr,
        .encoded_video_codecs = "h264",
        .encoded_audio_codecs = "opus",
        .raw_audio2           = nullptr,
        // .raw_audio2           = wowza_receive_multitrack_audio, // for multi-track
    };
#endif
}
