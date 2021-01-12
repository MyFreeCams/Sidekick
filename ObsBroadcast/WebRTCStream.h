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

#pragma once

#ifndef WEBRTC_STREAM_H_
#define WEBRTC_STREAM_H_

// silence warnings in MSVC
#ifdef _WIN32
#pragma warning (disable: 4189)  // local variable is initialized but not referenced
#endif

// project
#include "ADMWrapper.h"
#include "VideoTrackSource.h"

// solution
#include <websocket-client/WebsocketClient.h>

// obs
#include <libobs/obs.h>

// webrtc
#include "api/peer_connection_interface.h"
#include "api/rtp_sender_interface.h"
#include "api/scoped_refptr.h"
#include "api/set_local_description_observer_interface.h"
#include "api/set_remote_description_observer_interface.h"
#include "api/stats/rtc_stats_collector_callback.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread.h"
#include "rtc_base/thread_annotations.h"

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>


class LogSink : public rtc::LogSink
{
public:
    void OnLogMessage(const std::string& message) override;
};


class WebRTCStreamInterface
    : public WebsocketClient::Listener
    , public webrtc::PeerConnectionObserver
    , public webrtc::SetLocalDescriptionObserverInterface
    , public webrtc::SetRemoteDescriptionObserverInterface
    , public webrtc::RTCStatsCollectorCallback
{};


class WebRTCStream : public rtc::RefCountedObject<WebRTCStreamInterface>
{
public:
    explicit WebRTCStream(obs_output_t* output);
    ~WebRTCStream() override;

    bool Start();
    bool Stop(bool normal);

    void ResetStats();
    void ConfigureStreamParameters();
    bool CreatePeerConnection();
    bool AddTracks();
    bool OpenWebsocketConnection();
    void SendOffer(const webrtc::SessionDescriptionInterface* desc);
    void SetBitrate();

    void onAudioFrame(audio_data* frame);
    void onVideoFrame(video_data* frame);

    void GetStats();
    void LogLevel(rtc::LoggingSeverity logLevel);

    /// WebsocketClient::Listener implementation.
    void onConnected() override;
    void onAnswer(const std::string& sdp) override;
    void onRemoteIceCandidate(const std::string& candidate, const std::string& mid, int index) override;
    void onReadyToStartBroadcast() override;
    void onAuthFailure() override;
    void onConnectError() override;
    void onDisconnected() override;

    /// PeerConnectionObserver implementation.
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState state) override;
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState state) override;
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState /*new_state*/) override {}
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> /*channel*/) override {}
    void OnRenegotiationNeeded() override {}
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState /*new_state*/) override {}
    void OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& /*candidates*/) override {}
    void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/,
                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& /*streams*/) override {}
    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/) override {}

    /// SetLocalDescriptionObserverInterface implementation.
    void OnSetLocalDescriptionComplete(webrtc::RTCError error) override;

    /// SetRemoteDescriptionObserverInterface implementation.
    void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

    /// RTCStatsCollectorCallback implementation.
    void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;

    obs_output_t* output() const { return m_pOutput; }
    uint32_t height() const { return m_nHeight; }
    uint32_t width() const { return m_nWidth; }

    uint32_t videoBitrate() const { return video_bitrate_; }
    double currentFps() const { return outbound_fps_; }
    uint32_t currentHeight() const { return frame_height_; }
    uint32_t currentWidth() const { return frame_width_; }

    uint64_t totalBytesSent() const { return audio_bytes_sent_ + video_bytes_sent_; }
    uint32_t nackReceived() const { return nack_received_; }
    uint32_t pliReceived() const { return pli_received_; }
    uint32_t framesDropped() const { return frames_dropped_; }
    int packetsLost() const { return packets_lost_; }
    int packetsSent() const { return (int)packets_sent_; }

private:
    obs_output_t* m_pOutput;  // OBS stream output
    std::unique_ptr<WebsocketClient> m_pWsClient;

    int m_nWidth;
    int m_nHeight;
    int m_nVideoBitrateKbps;
    int m_nAudioBitrateKbps;
    int m_nFrameRate;
    std::string m_sVideoCodec;
    std::string m_sAudioCodec;
    std::string m_sVideoServer;
    std::string m_sProtocol;
    std::string m_sRegion;

    mutable webrtc::Mutex mutex_;

    int video_bitrate_bps_;
    int total_bitrate_bps_;
    uint16_t frame_id_;

    bool started_;
    bool stopping_;

    // Outbound RTP Stream Stats
    int64_t outbound_time_us_;
    uint32_t packets_sent_;
    uint64_t packets_rtx_;
    uint64_t audio_bytes_sent_;
    uint64_t video_bytes_sent_;
    uint64_t video_bytes_rtx_;
    uint32_t outbound_frames_sent_;
    double outbound_fps_;
    double total_pkt_send_delay_;

    uint32_t video_bitrate_;
    uint32_t avg_video_bitrate_;

    int64_t prev_video_bytes_;
    uint32_t prev_frames_sent_;
    int64_t  prev_timestamp_;

    // Media Stream Track Stats
    uint32_t frame_width_;
    uint32_t frame_height_;
    uint32_t frames_sent_;
    uint32_t huge_frames_sent_;
    uint32_t frames_dropped_;
    double frames_per_second_;

    // RTP Stream Stats
    uint32_t nack_received_;
    uint32_t pli_received_;
    uint32_t sli_received_;

    // Remote Inbound RTP Stream Stats
    int packets_lost_;
    int rtt_;
    double jitter_;

    std::thread close_async_;

    std::unique_ptr<rtc::Thread> network_;
    std::unique_ptr<rtc::Thread> worker_;
    std::unique_ptr<rtc::Thread> signaling_;

    rtc::scoped_refptr<ADMWrapper> adm_;
    rtc::scoped_refptr<webrtc::AudioProcessing> apm_;

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc_;

    rtc::scoped_refptr<webrtc::AudioSourceInterface> audioSource_;
    rtc::scoped_refptr<VideoTrackSource> videoSource_;

    rtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack_;
    rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack_;

    rtc::scoped_refptr<webrtc::RtpSenderInterface> audioSender_;
    rtc::scoped_refptr<webrtc::RtpSenderInterface> videoSender_;

    std::unique_ptr<LogSink> logSink_;
    rtc::LoggingSeverity logLevel_;
};

#endif  // WEBRTC_STREAM_H_
