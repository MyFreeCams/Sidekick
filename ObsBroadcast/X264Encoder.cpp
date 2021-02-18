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

#define WEBRTC_ADAPT_FRAME_ENABLED 0
#define X264ENC_ENABLE_RECONFIGURE 1
#define X264ENC_ENABLE_FIR 0
#define X264ENC_LOG_RTT 0
#define X264ENC_VERBOSE_LOG 0
#define X264ENC_TRELLIS 0

#include "X264Encoder.h"
#include "SanitizeInputs.h"
#include "webrtc_version.h"

#include <libPlugins/MFCConfigConstants.h>

#include "absl/types/optional.h"
#include "api/video/color_space.h"
#include "api/video/video_frame_buffer.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_coding/utility/simulcast_rate_allocator.h"
#include "modules/video_coding/utility/simulcast_utility.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/time_utils.h"
#include "system_wrappers/include/metrics.h"
#include "video/video_stream_encoder.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <cassert>

using std::string;
using std::vector;
using std::make_unique;
using std::unique_ptr;
using namespace webrtc;
using PrimaryID = ColorSpace::PrimaryID;
using TransferID = ColorSpace::TransferID;
using MatrixID = ColorSpace::MatrixID;
using RangeID = ColorSpace::RangeID;

#ifndef LOG_IMPL
#define LOG_IMPL(severity, format, ...)                                                                                \
    constexpr size_t bufferSize = 4096;                                                                                \
    char errorBuffer[bufferSize];                                                                                      \
    std::snprintf(errorBuffer, bufferSize, format, ##__VA_ARGS__);                                                     \
    RTC_LOG(severity) << errorBuffer
#endif

static const uint32_t kIDRIntervalSec   = MFC_SERVICES_JSON_KEYINT_VALUE;
static const uint32_t kMaxFramerate     = 30;

static const uint32_t kLongStartcodeSize  = 4;
static const uint32_t kShortStartcodeSize = 3;

static const float kHighQualityCrf   = 18.0;
static const float kNormalQualityCrf = 21.5;

// This frame height and greater will have CRF set to |kNormalQualityCrf|.
static const int kMinFrameHeightNormalQualityCrf = 720;
// Bitrate lower than this will have CRF set to |kNormalQualityCrf|.
static const int kMinBitrateKbpsHighQualityCrf = 1000;

// Divide bitrate by |kVbvBufferSizeFactor| to determine VBV buffer size.
static const int kVbvBufferSizeFactor = 2;

// Used by histograms. Values of entries should not be changed.
enum X264EncoderEvent
{
    kX264EncoderEventInit  = 0,
    kX264EncoderEventError = 1,
    kX264EncoderEventMax   = 16,
};


/// Round |num| to a multiple of |multiple|.
template<typename T>
inline T roundUp(T num, T multiple)
{
    assert(multiple);
    return ((num + multiple - 1) / multiple) * multiple;
}


static bool UseHqCrf(int frameHeight, int bitrateKbps)
{
    return (frameHeight < kMinFrameHeightNormalQualityCrf
            && bitrateKbps > kMinBitrateKbpsHighQualityCrf);
}


static VideoFrameType X264ToWebrtcFrameType(int type)
{
    switch (type)
    {
    case X264_TYPE_IDR:
    case X264_TYPE_KEYFRAME:
        return VideoFrameType::kVideoFrameKey;
    case X264_TYPE_I:
    case X264_TYPE_P:
    case X264_TYPE_B:
    case X264_TYPE_AUTO:
        return VideoFrameType::kVideoFrameDelta;
    default:
        break;
    }

    RTC_NOTREACHED() << "Unexpected/invalid frame type: " << type;
    return VideoFrameType::kEmptyFrame;
}


static VideoFrameType WebrtcFrameType(const x264_picture_t* pictureOut)
{
    if (pictureOut->b_keyframe)
        return VideoFrameType::kVideoFrameKey;

    return X264ToWebrtcFrameType(pictureOut->i_type);
}


static rtc::LoggingSeverity X264ToRtcLogLevel(int level)
{
    switch (level)
    {
    case X264_LOG_DEBUG:
        return rtc::LS_VERBOSE;
    case X264_LOG_INFO:
        return rtc::LS_INFO;
    case X264_LOG_WARNING:
        return rtc::LS_WARNING;
    case X264_LOG_ERROR:
        return rtc::LS_ERROR;
    default:
        return rtc::LS_NONE;
    }
}


static void X264Logger(void*, int level, const char* format, va_list args)
{
    auto severity = X264ToRtcLogLevel(level);
    constexpr size_t bufferSize = 4096;
    char errorBuffer[bufferSize]{};
    std::vsnprintf(errorBuffer, bufferSize, format, args);
    RTC_LOG_V(severity) << errorBuffer;
}


/// Helper method used by X264Encoder::Encode.
///
/// Fixes NALU start codes and copies the encoded bytes from |nal| to |encImg|.
///
/// After x264 encoding, the encoded bytes ("NAL units") are stored in |nal|. Each
/// NAL unit is a fragment starting with a 3-byte (00 00 01) or 4-byte (00 00 00 01)
/// start code.
///
/// LibWebRTC's H.264 decoder requires that all NALUs begin with a 4-byte start code.
/// x264 uses 4-byte start codes for SPS, PPS, and the initial NALU. 3-byte start codes
/// are used for all other NALUs.
///
/// All of this data (including the start codes) is copied to |encImg->_buffer|.
/// |encImg->_buffer| may be deleted and reallocated if a bigger buffer is required.
static void RtpFragmentize(EncodedImage* encImg, uint32_t numNals, x264_nal_t* nal)
{
    size_t requiredCapacity = 0;
    size_t fragmentsCount = 0;

    // Calculate minimum buffer size required to hold encoded data.
    for (uint32_t idx = 0; idx < numNals; ++idx, ++fragmentsCount)
    {
        RTC_CHECK_GE(nal[idx].i_payload, 0);

        // Ensure |requiredCapacity| will not overflow.
        RTC_CHECK_LE((size_t)nal[idx].i_payload, std::numeric_limits<size_t>::max() - requiredCapacity);

        requiredCapacity += nal[idx].i_payload;

        // When a 3-byte start code (00 00 01) is found, increment |requiredCapacity| by 1.
        if (!nal[idx].b_long_startcode)
            ++requiredCapacity;
    }

    // Allocate required bytes.
    auto buffer = EncodedImageBuffer::Create(requiredCapacity);
    encImg->SetEncodedData(buffer);

    const uint8_t startCode[4] = {0, 0, 0, 1};
    encImg->set_size(0);
    size_t frag = 0;
    size_t length = 0;

    // Iterate NAL units, note each NAL unit as a fragment and copy
    // the data to |encImg->_buffer|.
    //
    // Remove the start code generated by x264 and prepend a
    // 4-byte start code (00 00 00 01) in front of the NALU.
    for (uint32_t idx = 0; idx < numNals; ++idx, ++frag)
    {
        RTC_DCHECK_GE(nal[idx].i_payload, 4);

        uint32_t offset = !nal[idx].b_long_startcode ? kShortStartcodeSize : kLongStartcodeSize;
        uint32_t naluSize = nal[idx].i_payload - offset;

        // Copy the (4-byte) start code first.
        memcpy(buffer->data() + encImg->size(), startCode, sizeof(startCode));
        length += sizeof(startCode);
        encImg->set_size(length);

        // Copy the data without start code.
        memcpy(buffer->data() + encImg->size(), nal[idx].p_payload + offset, naluSize);
        length += naluSize;
        encImg->set_size(length);
    }
}


X264Encoder::X264Encoder(const cricket::VideoCodec& codec)
    : encoder_(nullptr)
    , encodedImageCallback_(nullptr)
    , packetizationMode_(H264PacketizationMode::SingleNalUnit)
    , width_(0)
    , height_(0)
    , maxFramerate_(0.0)
    , fps_(0)
    , bitrate_kbps_(0)
    , maxPayloadSize_(0)
    , paused_(false)
    , hasReportedInit_(false)
    , hasReportedError_(false)
    , frameCount_(0)
    , rtt_ms_(0)
{
    RTC_LOG(LS_INFO) << "Using x264 encoder";
    string pktModeStr;
    if (codec.GetParam(cricket::kH264FmtpPacketizationMode, &pktModeStr))
    {
        if (pktModeStr == "1")
            packetizationMode_ = H264PacketizationMode::NonInterleaved;
    }
}


X264Encoder::~X264Encoder()
{
    RTC_LOG_F(LS_INFO) << "dtor";
    Release();
}


// static
unique_ptr<X264Encoder> X264Encoder::Create(const cricket::VideoCodec& codec)
{
    return make_unique<X264Encoder>(codec);
}


// static
unique_ptr<X264Encoder> X264Encoder::Create(const SdpVideoFormat& format)
{
    return make_unique<X264Encoder>(cricket::VideoCodec(format));
}


int X264Encoder::InitEncode(const VideoCodec* inst, const VideoEncoder::Settings& settings)
{
    ReportInit();

    if (!inst || inst->codecType != kVideoCodecH264)
    {
        RTC_LOG_F(LS_ERROR) << "Error: Invalid codec or codec settings";
        ReportError();
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    if (inst->maxFramerate == 0)
    {
        RTC_LOG_F(LS_ERROR) << "Error: maxFramerate must be > 0";
        ReportError();
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    if (inst->width < 1 || inst->height < 1)
    {
        RTC_LOG_F(LS_ERROR) << "Error: width or height < 1";
        ReportError();
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    if (encoder_ != nullptr)
    {
        x264_encoder_close(encoder_);
        encoder_ = nullptr;
    }
    RTC_CHECK(!encoder_);

    width_          = inst->width;
    height_         = inst->height;
    fps_            = std::min(kMaxFramerate, inst->maxFramerate);
    maxFramerate_   = (double)fps_;
    maxPayloadSize_ = settings.max_payload_size;
    frameCount_     = 0;

    int videoBitrate = (int)inst->maxBitrate;
    // Ensure bitrate is reasonable for frame size.
    SanitizeInputs::ConstrainBitrate(height_, videoBitrate);
    bitrate_kbps_ = roundUp((uint32_t)videoBitrate, fps_);

    if (&codec_ != inst)
        codec_ = *inst;

    codec_.H264()->keyFrameInterval = static_cast<int>(fps_ * kIDRIntervalSec);

    // Code expects simulcastStream resolutions to be correct, make sure they are
    // filled even when there are no simulcast layers.
    if (codec_.numberOfSimulcastStreams == 0)
    {
        codec_.simulcastStream[0].width     = codec_.width;
        codec_.simulcastStream[0].height    = codec_.height;
    }

#if X264ENC_VERBOSE_LOG
    RTC_LOG(INFO) << "frame width:          " << width_;
    RTC_LOG(INFO) << "frame height:         " << height_;
    RTC_LOG(INFO) << "target bitrate:       " << bitrate_kbps_;
    RTC_LOG(INFO) << "max framerate:        " << maxFramerate_;
    RTC_LOG(INFO) << "max payload size:     " << maxPayloadSize_;
    RTC_LOG(INFO) << "frame dropping on:    " << codec_.H264()->frameDroppingOn;
    RTC_LOG(INFO) << "keyframe interval:    " << codec_.H264()->keyFrameInterval;
    RTC_LOG(INFO) << "simulcast streams:    " << codec_.numberOfSimulcastStreams;
    RTC_LOG(INFO) << "temporal layers:      " << codec_.simulcastStream[0].numberOfTemporalLayers;
#endif

    auto params = CreateEncoderParams();
    if (!params)
        return WEBRTC_VIDEO_CODEC_ERROR;

    // Allocate picture.
    int ret = x264_picture_alloc(&picIn_, params->i_csp, params->i_width, params->i_height);
    if (ret < 0)
    {
        RTC_LOG_F(LS_ERROR) << "Failed to allocate picture. errno: " << ret;
        ReportError();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // Create encoder.
    encoder_ = x264_encoder_open(params.get());
    if (!encoder_)
    {
        RTC_LOG_F(LS_ERROR) << "Failed to open x264 encoder";
        x264_picture_clean(&picIn_);
        ReportError();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // Initialize encoded image using the size of unencoded data for buffer capacity allocation.
    const size_t newCapacity = CalcBufferSize(VideoType::kYV12, width_, height_);

    image_.SetEncodedData(EncodedImageBuffer::Create(newCapacity));
    image_._encodedWidth  = (uint32_t)width_;
    image_._encodedHeight = (uint32_t)height_;
    image_.set_size(0);
    //image_.playout_delay_ = {0, 0};

    const ColorSpace cs(PrimaryID::kBT709, TransferID::kBT709, MatrixID::kBT709, RangeID::kLimited);
    image_.SetColorSpace(cs);

#if 0
    SimulcastRateAllocator init_allocator(codec_);
    auto datarate = DataRate::KilobitsPerSec(bitrate_kbps_);

    VideoBitrateAllocation allocation =
       init_allocator.Allocate(
           VideoBitrateAllocationParameters(datarate, maxFramerate_));
    allocation.SetBitrate(0, 0, bitrate_kbps_ * 1000);
    allocation.set_bw_limited(false);

    const auto rc_params =
       VideoEncoder::RateControlParameters(allocation, maxFramerate_, datarate);

    SetRates(rc_params);
#endif

    return WEBRTC_VIDEO_CODEC_OK;
}


unique_ptr<x264_param_t> X264Encoder::CreateEncoderParams()
{
    auto params = make_unique<x264_param_t>();

    int ret = x264_param_default_preset(params.get(), "veryfast", "zerolatency");
    if (ret != 0)
    {
        RTC_LOG_F(LS_ERROR) << "Failed to create x264 param defaults. code: " << ret;
        ReportError();
        return nullptr;
    }

    params->pf_log                  = X264Logger;
    params->i_log_level             = X264_LOG_DEBUG;

    params->i_bitdepth              = 8;
    params->i_csp                   = X264_CSP_NV12;
    params->b_annexb                = 1;
    params->b_repeat_headers        = 1;
    params->b_vfr_input             = 0;

    params->i_width                 = width_;
    params->i_height                = height_;
    params->i_fps_den               = 1;
    params->i_fps_num               = fps_;
    params->i_timebase_den          = 90000;
    params->i_timebase_num          = params->i_timebase_den * params->i_fps_den / params->i_fps_num;
    params->i_level_idc             = height_ > 720 ? 41 : 31;

    /// CPU flags.
    params->i_threads               = X264_THREADS_AUTO;
    params->b_sliced_threads        = 1;
    params->i_slice_max_size        = (int)maxPayloadSize_;  // Using single NALU per packet, limit slice size: MTU - overhead

    /// Bitstream parameters.
    params->b_intra_refresh         = 0;
    params->i_bframe                = 0;
#if X264ENC_ENABLE_RECONFIGURE
    params->i_nal_hrd               = X264_NAL_HRD_NONE;
#else
    params->i_nal_hrd               = X264_NAL_HRD_VBR;
#endif
    params->i_keyint_max            = static_cast<int>(fps_ * kIDRIntervalSec);
    params->i_keyint_min            = params->i_keyint_max / 2;
    params->i_scenecut_threshold    = 0;  // For consistent GOP

    /// Rate control.
    params->rc.i_rc_method          = X264_RC_ABR;
    params->rc.i_bitrate            = (int)bitrate_kbps_;
    params->rc.i_vbv_max_bitrate    = params->rc.i_bitrate;
    params->rc.i_vbv_buffer_size    = params->rc.i_bitrate / kVbvBufferSizeFactor;
    params->rc.f_rate_tolerance     = 0.1f;  // Minimizes inter-frame delay variance
    params->rc.b_filler             = 0;     // WebRTC's bitstream parser can't handle NAL unit 12

    if (params->rc.i_rc_method == X264_RC_CRF)
    {
        params->rc.f_rf_constant =
            UseHqCrf(height_, params->rc.i_bitrate) ? kHighQualityCrf : kNormalQualityCrf;
    }

    params->analyse.i_weighted_pred = 0;  // Not supported by WebRTC's bitstream parser

#if X264ENC_TRELLIS
    params->analyse.i_trellis       = 1;
    params->analyse.f_psy_trellis   = 0.15;
#endif

    params->vui.b_fullrange = 0;
    params->vui.i_colorprim = 1;  // BT.709-6
    params->vui.i_transfer  = 1;  // BT.709-6
    params->vui.i_colmatrix = 1;  // BT.709-6

    ret = x264_param_apply_profile(params.get(), "main");
    if (ret < 0)
    {
        RTC_LOG_F(LS_ERROR) << "Failed to apply x264 profile. code: " << ret;
        ReportError();
        return nullptr;
    }

    return params;  // Either copy elision takes place or std::move is implicitly applied
}


int32_t X264Encoder::Release()
{
    if (encoder_ != nullptr)
    {
        x264_encoder_close(encoder_);
        encoder_ = nullptr;
    }
    image_.ClearEncodedData();
    encodedImageCallback_ = nullptr;
    frameCount_ = 0;

    return WEBRTC_VIDEO_CODEC_OK;
}


int32_t X264Encoder::RegisterEncodeCompleteCallback(EncodedImageCallback* callback)
{
    RTC_LOG(LS_INFO) << __FUNCTION__;
    encodedImageCallback_ = callback;

    return WEBRTC_VIDEO_CODEC_OK;
}


void X264Encoder::SetRates(const RateControlParameters& parameters)
{
    if (!IsInitialized())
    {
        RTC_LOG_F(LS_INFO) << "SetRates() while uninitialized";
        return;
    }

    if (parameters.framerate_fps < 1.0)
        return;

    if (parameters.bitrate.get_sum_bps() == 0)
    {
        RTC_LOG_F(LS_INFO) << "Encoder paused";
        paused_ = true;
        return;
    }
    paused_ = false;

#if X264ENC_ENABLE_RECONFIGURE
    uint32_t newBitrateKbps = parameters.bitrate.get_sum_kbps();
    if (newBitrateKbps != bitrate_kbps_)
    {
        bitrate_kbps_ = newBitrateKbps;
        ReconfigureBitrate((int)bitrate_kbps_);
    }

    double newMaxFramerate = std::min((double)kMaxFramerate, parameters.framerate_fps);
    if (abs(newMaxFramerate - maxFramerate_) > 1.0)
    {
        maxFramerate_ = newMaxFramerate;
        codec_.maxFramerate = std::min(kMaxFramerate, static_cast<uint32_t>(parameters.framerate_fps + 0.5));
        fps_ = codec_.maxFramerate;
        ReconfigureFps(fps_);
    }
 #endif

    RTC_LOG_F(LS_INFO) << " fps: " << parameters.framerate_fps
                       << ", bitrate: " << parameters.bitrate.get_sum_kbps();
}


int32_t X264Encoder::Encode(const VideoFrame& inputFrame, const vector<VideoFrameType>* frameTypes)
{
    if (!IsInitialized())
    {
        RTC_LOG_F(LS_WARNING) << "InitEncode() has not been called";
        ReportError();
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (!encodedImageCallback_)
    {
        RTC_LOG_F(LS_WARNING) << "InitEncode() called before RegisterEncodeCompleteCallback()";
        ReportError();
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }

    if (paused_)
    {
        encodedImageCallback_->OnDroppedFrame(webrtc::EncodedImageCallback::DropReason::kDroppedByEncoder);
        return WEBRTC_VIDEO_CODEC_OK;
    }

    bool sendIDR = false;
#if WEBRTC_ADAPT_FRAME_ENABLED
    // Check if frame size has been changed by the AdaptFrame API.
    if (height_ != inputFrame.height())
    {
        RTC_LOG_F(LS_INFO) << "Frame size has been changed. Reconfiguring x264 encoder";
#if X264ENC_ENABLE_RECONFIGURE
        // Frame size can only be changed per-GOP. Send an IDR-frame to start a new GOP.
        sendIDR = true;

        width_ = inputFrame.width();
        height_ = inputFrame.height();

        if (!ReconfigureFrameSize(width_, height_))
            return WEBRTC_VIDEO_CODEC_ERROR;

        int videoBitrate = (int)bitrate_kbps_;
        // Ensure bitrate is reasonable for frame size.
        SanitizeInputs::ConstrainBitrate(height_, videoBitrate);
        if (videoBitrate != (int)bitrate_kbps_)
        {
            bitrate_kbps_ = (uint32_t)videoBitrate;
            if (!ReconfigureBitrate(videoBitrate))
                return WEBRTC_VIDEO_CODEC_ERROR;
        }
#endif
    }
#endif

    bool firRequest = false;
    if (frameTypes != nullptr)
    {
        // No SVC support (only supporting a single stream for now).
        RTC_DCHECK_EQ(frameTypes->size(), 1);

        // Check whether to skip this frame.
        if ((*frameTypes)[0] == VideoFrameType::kEmptyFrame)
        {
            encodedImageCallback_->OnDroppedFrame(webrtc::EncodedImageCallback::DropReason::kDroppedByEncoder);
            return WEBRTC_VIDEO_CODEC_OK;
        }
// Ignore FIR requests for now because Wowza sends an FIR packet every 1 second, not in response to packet loss.
#if X264ENC_ENABLE_FIR
        // Check if a Full Intra Refresh (FIR) is requested.
        firRequest = (*frameTypes)[0] == VideoFrameType::kVideoFrameKey;
#endif
    }

    picIn_                  = {0};
    picIn_.i_type           = (firRequest || sendIDR) ? X264_TYPE_IDR : X264_TYPE_AUTO;  // Send an IDR-frame on FIR request.
    picIn_.i_pts            = frameCount_;
    picIn_.img.i_csp        = X264_CSP_NV12;
    picIn_.img.i_plane      = 2;
    const auto frameBuffer  = inputFrame.video_frame_buffer()->GetNV12();
    picIn_.img.plane[0]     = const_cast<uint8_t*>(frameBuffer->DataY());
    picIn_.img.plane[1]     = const_cast<uint8_t*>(frameBuffer->DataUV());
    picIn_.img.i_stride[0]  = frameBuffer->StrideY();
    picIn_.img.i_stride[1]  = frameBuffer->StrideUV();

    int numNals = 0;
    x264_nal_t* nal = nullptr;
    x264_picture_t picOut{};

    // Encode one picture(frame).
    auto encode_start = rtc::TimeMillis();
    int encodedFrameSize = x264_encoder_encode(encoder_, &nal, &numNals, &picIn_, &picOut);
    if (encodedFrameSize < 0)
    {
        RTC_LOG_F(LS_ERROR) << "x264 frame encoding failed. encoded frame size: " << encodedFrameSize;
        Release();
        ReportError();
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    auto encode_finish = rtc::TimeMillis();

    //auto nalBuffer = (uint8_t*)malloc(nal->i_payload * 3 / 2 + 5 + 64);
    //x264_nal_encode(encoder_, nalBuffer, nal);

    image_._encodedWidth    = frameBuffer->width();
    image_._encodedHeight   = frameBuffer->height();
    image_._frameType       = WebrtcFrameType(&picOut);
    image_.ntp_time_ms_     = inputFrame.ntp_time_ms();
    image_.SetTimestamp(inputFrame.timestamp());
    image_.SetEncodeTime(encode_start, encode_finish);
    //image_.playout_delay_   = {0, 0};

    // Split encoded image into fragments and copy from |nal| to |image_|.
    RtpFragmentize(&image_, numNals, nal);
    image_.timing_.packetization_finish_ms = rtc::TimeMillis();

    // Encoder can skip frames to save bandwidth.
    if (image_.size() == 0)
    {
        RTC_LOG(INFO) << "ENCODER DROPPED FRAME";
        encodedImageCallback_->OnDroppedFrame(webrtc::EncodedImageCallback::DropReason::kDroppedByEncoder);
        return WEBRTC_VIDEO_CODEC_OK;
    }

    // Parse bitstream for QP.
    bitstreamParser_.ParseBitstream(image_);
    image_.qp_ = bitstreamParser_.GetLastSliceQp().value_or(-1);
    if (image_.qp_ < 0 || image_.qp_ > 51)
        image_.qp_ = picOut.i_qpplus1;

#if X264ENC_VERBOSE_LOG
    RTC_LOG(INFO) << "timestamp ntp:    " << image_.ntp_time_ms_;
    RTC_LOG(INFO) << "timestamp 90kHz:  " << image_.Timestamp();
    RTC_LOG(INFO) << "resolution:       " << width_ << "x" << height_;
    RTC_LOG(INFO) << "numNals:          " << numNals;
    RTC_LOG(INFO) << "enc frame size:   " << encodedFrameSize;
    RTC_LOG(INFO) << "enc image size:   " << image_.size();
    RTC_LOG(INFO) << "keyframe:         " << (picOut.b_keyframe ? "yes" : "no");
    RTC_LOG(INFO) << "crf:              " << picOut.prop.f_crf_avg;
    RTC_LOG(INFO) << "qp:               " << picOut.i_qpplus1;
    RTC_LOG(INFO) << "qp (last slice):  " << image_.qp_;
#endif

    CodecSpecificInfo codec_specific{};
    codec_specific.codecType                                = VideoCodecType::kVideoCodecH264;
    codec_specific.codecSpecific.H264.packetization_mode    = H264PacketizationMode::SingleNalUnit;
    codec_specific.codecSpecific.H264.temporal_idx          = kNoTemporalIdx;
    codec_specific.codecSpecific.H264.idr_frame             = static_cast<bool>(picOut.b_keyframe);
    codec_specific.codecSpecific.H264.base_layer_sync       = false;

    // Deliver encoded image.
    encodedImageCallback_->OnEncodedImage(image_, &codec_specific);

    ++frameCount_;

    return WEBRTC_VIDEO_CODEC_OK;
}


VideoEncoder::EncoderInfo X264Encoder::GetEncoderInfo() const
{
    VideoEncoder::EncoderInfo info{};
    info.requested_resolution_alignment = 2;
    info.has_trusted_rate_controller    = true;
    info.supports_native_handle         = true;
    info.implementation_name            = "x264";
    info.scaling_settings               = VideoEncoder::ScalingSettings::kOff;
    info.has_internal_source            = false;
    info.is_hardware_accelerated        = false;
    info.supports_simulcast             = false;
    //info.preferred_pixel_formats        = {VideoFrameBuffer::Type::kNV12};
    return info;
}


bool X264Encoder::ReconfigureFps(uint32_t fps)
{
    x264_param_t params{};
    x264_encoder_parameters(encoder_, &params);

    params.i_fps_num    = fps;
    params.i_fps_den    = 1;
    params.i_keyint_max = static_cast<int>(fps * kIDRIntervalSec);
    params.i_keyint_min = params.i_keyint_max / 2;

    int ret = x264_encoder_reconfig(encoder_, &params);
    if (ret < 0)
    {
        RTC_LOG_F(LS_WARNING) << "Failed to reconfigure encoder (fps). code: " << ret;
        return false;
    }

    return true;
}


bool X264Encoder::ReconfigureBitrate(int bitrateKbps)
{
    x264_param_t params{};
    x264_encoder_parameters(encoder_, &params);

    // Reset rate control parameters.
    params.rc.i_bitrate = bitrateKbps;

    // VBV cannot be reconfigured if using NAL-HRD.
    if (params.i_nal_hrd == X264_NAL_HRD_NONE)
    {
        params.rc.i_vbv_max_bitrate = params.rc.i_bitrate;
        params.rc.i_vbv_buffer_size = params.rc.i_bitrate / kVbvBufferSizeFactor;
    }

    if (params.rc.i_rc_method == X264_RC_CRF)
    {
        params.rc.f_rf_constant =
            UseHqCrf(height_, bitrateKbps) ? kHighQualityCrf : kNormalQualityCrf;
    }

    int ret = x264_encoder_reconfig(encoder_, &params);
    if (ret < 0)
    {
        RTC_LOG_F(LS_WARNING) << "Failed to reconfigure encoder (bitrate). code: " << ret;
        return false;
    }

    return true;
}


bool X264Encoder::ReconfigureFrameSize(int width, int height)
{
    x264_param_t params{};
    x264_encoder_parameters(encoder_, &params);

    params.i_width  = width;
    params.i_height = height;

    int ret = x264_encoder_reconfig(encoder_, &params);
    if (ret < 0)
    {
        RTC_LOG_F(LS_WARNING) << "Failed to reconfigure encoder (frame size). code: " << ret;
        return false;
    }

    return true;
}


bool X264Encoder::IsInitialized() const
{
    return encoder_ != nullptr;
}


void X264Encoder::ReportInit()
{
    if (hasReportedInit_)
        return;
    RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.X264Encoder.Event", kX264EncoderEventInit,
                              kX264EncoderEventMax);
    hasReportedInit_ = true;
}


void X264Encoder::ReportError()
{
    if (hasReportedError_)
        return;
    RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.X264Encoder.Event", kX264EncoderEventError,
                              kX264EncoderEventMax);
    hasReportedError_ = true;
}


void X264Encoder::OnPacketLossRateUpdate(float packet_loss_rate)
{
    if (packet_loss_rate > 0.0f)
    {
        RTC_LOG(INFO) << "packet loss rate: " << packet_loss_rate;
    }
}


void X264Encoder::OnRttUpdate(int64_t rtt_ms)
{
    if (rtt_ms > 0 || rtt_ms != rtt_ms_)
    {
        int64_t delta = rtt_ms - rtt_ms_;
        string plus_minus = delta < 0 ? "" : "+";
#if X264ENC_LOG_RTT
        RTC_LOG(INFO) << "rtt (ms): " << rtt_ms << " (" << plus_minus << delta << ")";
#endif
    }
    rtt_ms_ = rtt_ms;
}


void X264Encoder::OnLossNotification(const VideoEncoder::LossNotification& loss_notification)
{
    RTC_LOG(INFO) << "time of last decodable:  " << loss_notification.timestamp_of_last_decodable;
    RTC_LOG(INFO) << "time of last received:   " << loss_notification.timestamp_of_last_received;
    RTC_LOG(INFO) << "last received decodable? " << (loss_notification.last_received_decodable ? "yes" : "no");
    RTC_LOG(INFO) << "dependencies decodable?  " << (loss_notification.dependencies_of_last_received_decodable ? "yes" : "no");
}
