/*
 * Copyright (c) 2013-2020 MFCXY, Inc. <mfcxy@mfcxy.com>
 * Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
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

#ifndef VIDEO_TRACK_SOURCE_H_
#define VIDEO_TRACK_SOURCE_H_

#include <webrtc_version.h>

#include "absl/types/optional.h"
#include "api/media_stream_interface.h"
#include "api/notifier.h"
#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_sink_interface.h"
#include "api/video/video_source_interface.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread_annotations.h"

using absl::optional;
using rtc::scoped_refptr;
using rtc::VideoSinkInterface;
using rtc::VideoSinkWants;
using std::vector;
using namespace webrtc;


/// Base class for sources without video adaptation.
/// Sinks must be added and removed on one and only one thread,
/// while OnFrame may be called on any thread.
class VideoTrackSource
    : public Notifier<VideoTrackSourceInterface>
    , public VideoSinkInterface<VideoFrame>
{
public:
    VideoTrackSource();
    ~VideoTrackSource() override;

    //// Delete copy constructor & copy assignment operator
    //VideoTrackSource(const VideoTrackSource&) = delete;
    //VideoTrackSource& operator=(const VideoTrackSource&) = delete;

    //VideoTrackSource(VideoTrackSource&&) = delete;
    //VideoTrackSource& operator=(VideoTrackSource&&) = delete;

    static scoped_refptr<VideoTrackSource> Create();

    bool KeepFrame(int64_t frameTimeNanos);
    void onIncomingData(const uint8_t* dataY, uint32_t strideY,
                        const uint8_t* dataUV, uint32_t strideUV,
                        int64_t timestamp_us, uint16_t frameId,
                        int width, int height,
                        VideoRotation videoRotation, VideoType videoType);

    /// VideoTrackSourceInterface implementation.
    bool is_screencast() const override { return false; }
    optional<bool> needs_denoising() const override { return optional<bool>(false); }
    bool GetStats(Stats* stats) override;
    bool SupportsEncodedOutput() const override { return false; }
    void GenerateKeyFrame() override {}
    void AddEncodedSink(VideoSinkInterface<RecordableEncodedFrame>* sink) override {}
    void RemoveEncodedSink(VideoSinkInterface<RecordableEncodedFrame>* sink) override {}

    /// MediaSourceInterface implementation.
    bool remote() const override { return false; }
    SourceState state() const override { return kLive; }

    /// VideoSourceInterface implementation.
    void AddOrUpdateSink(VideoSinkInterface<VideoFrame>* sink, const VideoSinkWants& wants) override;
    void RemoveSink(VideoSinkInterface<VideoFrame>* sink) override;

    /// VideoSinkInterface implementation.
    void OnFrame(const VideoFrame& frame) override;
    void OnDiscardedFrame() override;

protected:
    struct SinkPair
    {
        SinkPair(VideoSinkInterface<VideoFrame>* sink, VideoSinkWants wants)
            : sink(sink), wants(wants)
        {}
        VideoSinkInterface<VideoFrame>* sink;
        VideoSinkWants wants;
    };

    SinkPair* FindSinkPair(const VideoSinkInterface<VideoFrame>* sink);
    const vector<SinkPair>& sink_pairs() const { return sinks_; }

    void UpdateWants() RTC_EXCLUSIVE_LOCKS_REQUIRED(sinks_and_wants_mutex_);

    const scoped_refptr<VideoFrameBuffer>& GetBlackFrameBuffer(int width, int height)
        RTC_EXCLUSIVE_LOCKS_REQUIRED(sinks_and_wants_mutex_);

    bool frame_wanted() const;
    VideoSinkWants wants() const;

private:
    Mutex next_frame_mutex_;
    mutable Mutex sinks_and_wants_mutex_;
    Mutex stats_mutex_;

    optional<int64_t> nextFrameTimeNanos_ RTC_GUARDED_BY(next_frame_mutex_);
    optional<Stats> stats_ RTC_GUARDED_BY(stats_mutex_);

    scoped_refptr<VideoFrameBuffer> black_frame_buffer_;
    VideoSinkWants current_wants_ RTC_GUARDED_BY(sinks_and_wants_mutex_);
    bool previous_frame_sent_to_all_sinks_ RTC_GUARDED_BY(sinks_and_wants_mutex_) = true;

    vector<SinkPair> sinks_;
};

#endif  // VIDEO_TRACK_SOURCE_H_
