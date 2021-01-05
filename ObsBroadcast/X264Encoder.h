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

#ifndef X264_ENCODER_H_
#define X264_ENCODER_H_

#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "common_video/h264/h264_bitstream_parser.h"
#include "media/base/codec.h"
#include "media/base/h264_profile_level_id.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/h264/include/h264_globals.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "rtc_base/synchronization/mutex.h"

#ifdef _WIN32
#ifndef X264_API_IMPORTS
#define X264_API_IMPORTS
#endif
#endif

// stdint.h must be included before x264.h
#include <cstdint>
#include <x264.h>

#include <functional>
#include <memory>

using std::unique_ptr;
using std::vector;
using namespace webrtc;


class X264Encoder : public webrtc::H264Encoder
{
public:
    explicit X264Encoder(const cricket::VideoCodec& codec);
    ~X264Encoder() override;

    static unique_ptr<X264Encoder> Create(const cricket::VideoCodec& codec);
    static unique_ptr<X264Encoder> Create(const webrtc::SdpVideoFormat& format);

    /// VideoEncoder implementation.
    int InitEncode(const webrtc::VideoCodec* videoCodec,
                   const webrtc::VideoEncoder::Settings& settings) override;
    int32_t Release() override;

    int32_t RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) override;
    void SetRates(const webrtc::VideoEncoder::RateControlParameters& parameters) override;

    // The result of encoding - an EncodedImage and CodecSpecificInfo - are
    // passed to the encode complete callback.
    int32_t Encode(const webrtc::VideoFrame& frame,
                   const vector<webrtc::VideoFrameType>* frameTypes) override;
    webrtc::VideoEncoder::EncoderInfo GetEncoderInfo() const override;

    void OnPacketLossRateUpdate(float packet_loss_rate) override;
    void OnRttUpdate(int64_t rtt_ms) override;
    void OnLossNotification(const webrtc::VideoEncoder::LossNotification& loss_notification) override;
#if 0
    void SetFecControllerOverride(FecControllerOverride* fec_controller_override) override {};
#endif

    unique_ptr<x264_param_t> CreateEncoderParams();

    // Exposed for testing.
    webrtc::H264PacketizationMode PacketizationModeForTesting() const { return packetizationMode_; }

private:
    bool ReconfigureFps(uint32_t fps);
    bool ReconfigureBitrate(int bitrateKbps);
    bool ReconfigureFrameSize(int width, int height);
    bool IsInitialized() const;
    void ReportInit();
    void ReportError();

    x264_t*                         encoder_ = nullptr;
    x264_picture_t                  picIn_;

    webrtc::EncodedImage            image_;
    webrtc::EncodedImageCallback*   encodedImageCallback_ = nullptr;
    webrtc::H264BitstreamParser     bitstreamParser_;
    webrtc::H264PacketizationMode   packetizationMode_;
    webrtc::VideoCodec              codec_;

    int         width_              = 0;
    int         height_             = 0;
    double      maxFramerate_       = 0.0;
    uint32_t    fps_                = 0;
    uint32_t    bitrate_kbps_       = 0;
    uint32_t    maxBitrateKbps_     = 0;
    size_t      maxPayloadSize_     = 0;

    bool        paused_             = false;
    bool        hasReportedInit_    = false;
    bool        hasReportedError_   = false;

    int64_t     frameCount_         = 0;
    int64_t     rtt_ms_             = 0;
};

#endif  // X264_ENCODER_H_
