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

#include "VideoTrackSource.h"
#include "NV12Buf.h"

#include <libPlugins/MFCConfigConstants.h>

#include "absl/algorithm/container.h"
#include "api/video/color_space.h"
#include "api/video/encoded_image.h"
#include "api/video/i420_buffer.h"
#include "api/video/nv12_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"
#include "media/base/video_common.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/time_utils.h"
#include "system_wrappers/include/clock.h"
#include "third_party/libyuv/include/libyuv.h"

#include <algorithm>
#include <utility>

using rtc::RefCountedObject;

static const int MAX_WIDTH = MFC_SERVICES_JSON_MAX_WIDTH_VALUE;
static const int MAX_HEIGHT = MFC_SERVICES_JSON_MAX_HEIGHT_VALUE;
static const int MAX_FPS = 30;
static const int REQUIRED_ALIGNMENT = 2;


VideoTrackSource::VideoTrackSource()
{
    RTC_LOG(INFO) << __FUNCTION__;
}


VideoTrackSource::~VideoTrackSource()
{
    RTC_LOG(INFO) << __FUNCTION__;
}


// static
scoped_refptr<VideoTrackSource> VideoTrackSource::Create()
{
    return new RefCountedObject<VideoTrackSource>();
}


bool VideoTrackSource::KeepFrame(int64_t frameTimeNanos)
{
    MutexLock lock(&next_frame_mutex_);

    constexpr int64_t FRAME_INTERVAL_NANOS = rtc::kNumNanosecsPerSec / MAX_FPS;

    if (nextFrameTimeNanos_)
    {
        // Time until next frame should be outputted.
        const int64_t timeUntilNextFrameNanos = (*nextFrameTimeNanos_ - frameTimeNanos);
        // Continue if timestamp is within expected range.
        if (abs(timeUntilNextFrameNanos) < 2 * FRAME_INTERVAL_NANOS)
        {
            // Drop if a frame shouldn't be outputted yet.
            if (timeUntilNextFrameNanos > 0)
                return false;
            // Time to output new frame.
            *nextFrameTimeNanos_ += FRAME_INTERVAL_NANOS;
        }
        return true;
    }
    // First timestamp received or timestamp is way outside expected range, so
    // reset. Set first timestamp target to just half the interval to prefer
    // keeping frames in case of jitter.
    nextFrameTimeNanos_ = frameTimeNanos + FRAME_INTERVAL_NANOS / 2;
    return true;
}


void VideoTrackSource::onIncomingData(const uint8_t* dataY, uint32_t strideY,
                                      const uint8_t* dataUV, uint32_t strideUV,
                                      int64_t frameTimeNanos, uint16_t frameId,
                                      int width, int height,
                                      VideoRotation videoRotation)
{
    if (!KeepFrame(frameTimeNanos))
        return;

    auto buffer = NV12Buf::Create(dataY, strideY, dataUV, strideUV, width, height);

    auto frameTimeMicros = frameTimeNanos / rtc::kNumNanosecsPerMicrosec;
    auto timestampRtp = static_cast<uint32_t>(frameTimeMicros * 90 / rtc::kNumMicrosecsPerMillisec);

    auto clock = Clock::GetRealTimeClock();
    auto ntpTimeMs = clock->CurrentNtpInMilliseconds();

    ColorSpace cs(ColorSpace::PrimaryID::kBT709,
                  ColorSpace::TransferID::kBT709,
                  ColorSpace::MatrixID::kBT709,
                  ColorSpace::RangeID::kLimited);

    OnFrame(VideoFrame::Builder()
            .set_video_frame_buffer(buffer)
            .set_rotation(videoRotation)
            .set_timestamp_us(frameTimeMicros)
            .set_timestamp_rtp(timestampRtp)
            .set_ntp_time_ms(ntpTimeMs)
            .set_color_space(&cs)
            .set_id(frameId)
            .build());
}


bool VideoTrackSource::GetStats(VideoTrackSourceInterface::Stats* stats)
{
    MutexLock lock(&stats_mutex_);

    if (!stats_)
        return false;

    *stats = *stats_;
    return true;
}


void VideoTrackSource::AddOrUpdateSink(VideoSinkInterface<VideoFrame>* sink, const VideoSinkWants& wants)
{
    RTC_DCHECK(sink != nullptr);

    auto copy = VideoSinkWants(wants);
    if (wants.resolution_alignment < REQUIRED_ALIGNMENT)
        copy.resolution_alignment = REQUIRED_ALIGNMENT;
    if (wants.max_framerate_fps > MAX_FPS)
        copy.max_framerate_fps = MAX_FPS;
    if (wants.max_pixel_count > MAX_WIDTH * MAX_HEIGHT)
        copy.max_pixel_count = MAX_WIDTH * MAX_HEIGHT;

    MutexLock lock(&sinks_and_wants_mutex_);

    SinkPair* sink_pair = FindSinkPair(sink);
    if (!sink_pair)
    {
        // |Sink| is a new sink, which didn't receive previous frame.
        previous_frame_sent_to_all_sinks_ = false;
        sinks_.push_back(SinkPair(sink, copy));
        //sinks_.push_back(SinkPair(sink, wants));
    }
    else
    {
        sink_pair->wants = copy;
        //sink_pair->wants = wants;
    }
    
    UpdateWants();
}


void VideoTrackSource::RemoveSink(VideoSinkInterface<VideoFrame>* sink)
{
    RTC_DCHECK(sink != nullptr);
    MutexLock lock(&sinks_and_wants_mutex_);
    RTC_DCHECK(FindSinkPair(sink));

    sinks_.erase(
        std::remove_if(sinks_.begin(), sinks_.end(),
                       [sink](const SinkPair& sink_pair) { return sink_pair.sink == sink; }),
        sinks_.end());

    UpdateWants();
}


void VideoTrackSource::OnFrame(const VideoFrame& frame)
{
    MutexLock lock(&sinks_and_wants_mutex_);
    bool current_frame_was_discarded = false;

    for (auto& sink_pair : sink_pairs())
    {
        if (sink_pair.wants.rotation_applied && frame.rotation() != kVideoRotation_0)
        {
            // Calls to OnFrame are not synchronized with changes to the sink wants.
            // When rotation_applied is set to true, one or a few frames may get here
            // with rotation still pending. Protect sinks that don't expect any
            // pending rotation.
            RTC_LOG(LS_VERBOSE) << "Discarding frame with unexpected rotation.";
            sink_pair.sink->OnDiscardedFrame();
            current_frame_was_discarded = true;
            continue;
        }

        if (sink_pair.wants.black_frames)
        {
            VideoFrame black_frame =
                VideoFrame::Builder()
                    .set_video_frame_buffer(
                        GetBlackFrameBuffer(frame.width(), frame.height()))
                            .set_rotation(frame.rotation())
                            .set_timestamp_us(frame.timestamp_us())
                            .set_id(frame.id())
                            .build();
            sink_pair.sink->OnFrame(black_frame);
        }
        else if (!previous_frame_sent_to_all_sinks_ && frame.has_update_rect())
        {
            // Since last frame was not sent to some sinks, no reliable update
            // information is available, so we need to clear the update rect.
            VideoFrame copy = frame;
            copy.clear_update_rect();
            sink_pair.sink->OnFrame(copy);
        }
        else
        {
            sink_pair.sink->OnFrame(frame);
        }
    }

    previous_frame_sent_to_all_sinks_ = !current_frame_was_discarded;
}


void VideoTrackSource::OnDiscardedFrame()
{
    for (auto& sink_pair : sink_pairs())
    {
        sink_pair.sink->OnDiscardedFrame();
    }
}


VideoTrackSource::SinkPair* VideoTrackSource::FindSinkPair(const VideoSinkInterface<VideoFrame>* sink)
{
    auto sink_pair_it =
        absl::c_find_if(sinks_, [sink](const SinkPair& sink_pair) { return sink_pair.sink == sink; });

    if (sink_pair_it != sinks_.end())
        return &*sink_pair_it;

    return nullptr;
}


void VideoTrackSource::UpdateWants()
{
    VideoSinkWants wants;
    wants.max_framerate_fps = MAX_FPS;
    wants.max_pixel_count = MAX_WIDTH * MAX_HEIGHT;
    wants.rotation_applied = false;
    wants.resolution_alignment = REQUIRED_ALIGNMENT;

    for (auto& sink_pair : sink_pairs())
    {
        if (!sink_pair.sink)
            continue;

        if (sink_pair.wants.rotation_applied)
            wants.rotation_applied = true;

        if (sink_pair.wants.max_pixel_count < wants.max_pixel_count)
            wants.max_pixel_count = sink_pair.wants.max_pixel_count;

        // Select the minimum requested target_pixel_count, if any, of all sinks so
        // that we don't over utilize the resources for any one.
        if (sink_pair.wants.target_pixel_count
            && (!wants.target_pixel_count || (*sink_pair.wants.target_pixel_count < *wants.target_pixel_count)))
        {
            wants.target_pixel_count = sink_pair.wants.target_pixel_count;
        }

        // Select the minimum for the requested max framerates.
        if (sink_pair.wants.max_framerate_fps < wants.max_framerate_fps)
            wants.max_framerate_fps = sink_pair.wants.max_framerate_fps;

        if (sink_pair.wants.resolution_alignment > 1)
        {
            wants.resolution_alignment =
                cricket::LeastCommonMultiple(
                    wants.resolution_alignment, sink_pair.wants.resolution_alignment);
        }
    }

    if (wants.target_pixel_count && *wants.target_pixel_count >= wants.max_pixel_count)
        wants.target_pixel_count.emplace(wants.max_pixel_count);

    current_wants_ = wants;
}


const scoped_refptr<VideoFrameBuffer>& VideoTrackSource::GetBlackFrameBuffer(int width, int height)
{
    if (!black_frame_buffer_ || black_frame_buffer_->width() != width || black_frame_buffer_->height() != height)
    {
        scoped_refptr<I420Buffer> buffer = I420Buffer::Create(width, height);
        I420Buffer::SetBlack(buffer.get());
        black_frame_buffer_ = buffer;
    }

    return black_frame_buffer_;
}


bool VideoTrackSource::frame_wanted() const
{
    MutexLock lock(&sinks_and_wants_mutex_);
    return !sink_pairs().empty();
}


VideoSinkWants VideoTrackSource::wants() const
{
    MutexLock lock(&sinks_and_wants_mutex_);
    return current_wants_;
}
