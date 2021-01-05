#pragma once

#ifndef VIDEO_SEND_STREAM_H_
#define VIDEO_SEND_STREAM_H_

#include <memory>
#include <string>
#include <vector>

#include "api/transport/field_trial_based_config.h"
#include "api/video/video_frame.h"
#include "call/call.h"
#include "call/flexfec_receive_stream.h"
#include "call/test/mock_rtp_transport_controller_send.h"
#include "call/video_send_stream.h"
#include "rtc_base/buffer.h"


class FakeVideoSendStream final
    : public webrtc::VideoSendStream
    , public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
 public:
    FakeVideoSendStream(webrtc::VideoSendStream::Config config, webrtc::VideoEncoderConfig encoder_config);
    ~FakeVideoSendStream() override;

    const webrtc::VideoSendStream::Config& GetConfig() const;
    const webrtc::VideoEncoderConfig& GetEncoderConfig() const;
    const std::vector<webrtc::VideoStream>& GetVideoStreams() const;

    bool IsSending() const;
    bool GetVp8Settings(webrtc::VideoCodecVP8* settings) const;
    bool GetVp9Settings(webrtc::VideoCodecVP9* settings) const;
    bool GetH264Settings(webrtc::VideoCodecH264* settings) const;

    int GetNumberOfSwappedFrames() const;
    int GetLastWidth() const;
    int GetLastHeight() const;
    int64_t GetLastTimestamp() const;

    void SetStats(const webrtc::VideoSendStream::Stats& stats) { stats_ = stats; }

    void InjectVideoSinkWants(const rtc::VideoSinkWants& wants);

    int num_encoder_reconfigurations() const { return num_encoder_reconfigurations_; }
    bool resolution_scaling_enabled() const  { return resolution_scaling_enabled_; }
    bool framerate_scaling_enabled() const { return framerate_scaling_enabled_; }
    rtc::VideoSourceInterface<webrtc::VideoFrame>* source() const { return source_; }

 private:
    // rtc::VideoSinkInterface<VideoFrame> implementation.
    void OnFrame(const webrtc::VideoFrame& frame) override;

    // webrtc::VideoSendStream implementation.
    void UpdateActiveSimulcastLayers(const std::vector<bool> active_layers) override;

    void Start() override;
    void Stop() override;

    void AddAdaptationResource(rtc::scoped_refptr<webrtc::Resource> resource) override {};

    std::vector<rtc::scoped_refptr<webrtc::Resource>> GetAdaptationResources() override;

    void SetSource(rtc::VideoSourceInterface<webrtc::VideoFrame>* source,
                   const webrtc::DegradationPreference& degradation_preference) override;

    webrtc::VideoSendStream::Stats GetStats() override { return stats_; }

    void ReconfigureVideoEncoder(webrtc::VideoEncoderConfig config) override;

    bool sending_;
    webrtc::VideoSendStream::Config config_;
    webrtc::VideoEncoderConfig encoder_config_;
    std::vector<webrtc::VideoStream> video_streams_;
    rtc::VideoSinkWants sink_wants_;
    bool codec_settings_set_;

    union CodecSpecificSettings
    {
        webrtc::VideoCodecVP8 vp8;
        webrtc::VideoCodecVP9 vp9;
        webrtc::VideoCodecH264 h264;
    } codec_specific_settings_;

    bool resolution_scaling_enabled_;
    bool framerate_scaling_enabled_;
    rtc::VideoSourceInterface<webrtc::VideoFrame>* source_;
    int num_swapped_frames_;
    absl::optional<webrtc::VideoFrame> last_frame_;
    webrtc::VideoSendStream::Stats stats_;
    int num_encoder_reconfigurations_ = 0;
};

#endif  // VIDEO_SEND_STREAM_H_