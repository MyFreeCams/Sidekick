/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
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

#define MFC_LIMIT_WEBRTC_BITRATE_2500 1
#define WEBRTCSTREAM_MODIFY_SENDER_PARAMETERS 0
#define WEBRTCSTREAM_USE_BITRATE_SETTINGS 0

#ifdef _WIN32
#pragma warning (disable: 4003)  // enough actual parameters for macro
#pragma warning (disable: 4005)  // macro redefinition
#pragma warning (disable: 4840)  // non-portable use of class type as an argument to a variadic function
#endif

// project
#include "WebRTCStream.h"
#include "EncoderFactory.h"
#include "MFCEdgeIngest.h"
#include "MfcOauthApiCtx.h"
#include "ObsBroadcast.h"
#include "SanitizeInputs.h"
#include "SDPUtil.h"
#include "X264Encoder.h"
#include "webrtc_version.h"

// solution
#include <libPlugins/build_version.h>
#include <libPlugins/ObsUtil.h>

// obs
#include <media-io/video-io.h>

// webrtc
#include "api/create_peerconnection_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_codecs/audio_format.h"
#include "api/stats/rtcstats_objects.h"
#include "api/transport/bitrate_settings.h"
#include "api/video/video_bitrate_allocator.h"
#include "api/video/video_bitrate_allocator_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
//#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "pc/webrtc_sdp.h"
#include "third_party/libyuv/include/libyuv.h"

#include <QMessageBox>
#include <QString>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <sstream>

// use REST API instead of Sidekick for MFC authentication
#undef SIDEKICK_APP_VERSION  // TODO: delete when done testing REST API

#ifndef MFC_API_CLIENT_ID
#define MFC_API_CLIENT_ID "splitcam"
#endif

#define obs_debug(format, ...)  blog(400, format, ##__VA_ARGS__)
#define obs_info(format, ...)   blog(300, format, ##__VA_ARGS__)
#define obs_warn(format, ...)   blog(200, format, ##__VA_ARGS__)
#define obs_error(format, ...)  blog(100, format, ##__VA_ARGS__)

using rtc::LoggingSeverity;
using rtc::RefCountedObject;
using rtc::scoped_refptr;
using rtc::Thread;
using std::make_unique;
using std::string;
using std::unique_ptr;
using std::vector;
using namespace webrtc;
using PCI = webrtc::PeerConnectionInterface;
using PCFI = webrtc::PeerConnectionFactoryInterface;

extern CBroadcastCtx g_ctx;  // part of MFCLibPlugins.lib::MfcPluginAPI.obj
extern MfcOauthApiCtx g_apiCtx;

/// Round |num| to a multiple of |multiple|.
template<typename T>
inline T roundUp(T num, T multiple)
{
    assert(multiple);
    return ((num + multiple - 1) / multiple) * multiple;
}


int ui_MsgBox(const char* pszTitle, const char* pszBody, int nButtons, QWidget* pParent)
{
    QMessageBox mb(QMessageBox::Information,
                   pszTitle,
                   pszBody,
                   QMessageBox::StandardButtons(nButtons),
                   pParent);

    mb.setButtonText(QMessageBox::Ok, QString::fromUtf8("OK"));
    mb.setDefaultButton(QMessageBox::Ok);
    return mb.exec();
}


static unique_ptr<Thread> CreateNetwork()
{
    auto network = Thread::CreateWithSocketServer();
    network->SetName("WebRTCStream_Network", nullptr);
    network->Start();
    return network;
}


static unique_ptr<Thread> CreateWorker()
{
    auto worker = Thread::Create();
    worker->SetName("WebRTCStream_Worker", nullptr);
    worker->Start();
    return worker;
}


static unique_ptr<Thread> CreateSignaling()
{
    auto signaling = Thread::Create();
    signaling->SetName("WebRTCStream_Signaling", nullptr);
    signaling->Start();
    return signaling;
}


static scoped_refptr<ADMWrapper> CreateADM(Thread* worker)
{
    return worker->Invoke<scoped_refptr<ADMWrapper>>(
        RTC_FROM_HERE, []() { return ADMWrapper::Create(); });
}


static void ConfigureAPM(scoped_refptr<AudioProcessing> apm)
{
    AudioProcessing::Config config;
    config.pipeline.maximum_internal_processing_rate    = 48000;
    config.pipeline.multi_channel_capture               = false;
    config.pipeline.multi_channel_render                = false;
    config.echo_canceller.enabled                       = false;
    config.echo_canceller.mobile_mode                   = false;
    config.echo_canceller.enforce_high_pass_filtering   = false;
    config.gain_controller1.enabled                     = false;
    config.gain_controller2.enabled                     = false;
    config.gain_controller2.adaptive_digital.enabled    = false;
    config.gain_controller2.fixed_digital.gain_db       = 0;
    config.high_pass_filter.enabled                     = false;
    config.high_pass_filter.apply_in_full_band          = false;
    config.noise_suppression.enabled                    = false;
    config.noise_suppression.level                      = AudioProcessing::Config::NoiseSuppression::kLow;
    config.pre_amplifier.enabled                        = false;
    config.residual_echo_detector.enabled               = false;
    config.transient_suppression.enabled                = false;
    config.voice_detection.enabled                      = false;
    config.level_estimation.enabled                     = false;

    apm->ApplyConfig(config);
}


static scoped_refptr<AudioProcessing> CreateAPM()
{
    scoped_refptr<AudioProcessing> apm(AudioProcessingBuilder().Create());
    ConfigureAPM(apm);
    return apm;
}


static scoped_refptr<PCFI> CreatePCFactory(Thread* network, Thread* worker, Thread* signaling,
                                           scoped_refptr<ADMWrapper> adm, scoped_refptr<AudioProcessing> apm)
{
    return CreatePeerConnectionFactory(network, worker, signaling, adm, CreateBuiltinAudioEncoderFactory(),
                                       CreateBuiltinAudioDecoderFactory(), CreateX264EncoderFactory(),
                                       CreateBuiltinVideoDecoderFactory(), nullptr, apm, nullptr);
}


void LogSink::OnLogMessage(const string& message)
{
    obs_info("%s", message.c_str());
}


WebRTCStream::WebRTCStream(obs_output_t* output)
    : m_pOutput(output)
    , m_pWsClient(nullptr)
    , m_nWidth(0)
    , m_nHeight(0)
    , m_nVideoBitrateKbps(0)
    , m_nAudioBitrateKbps(0)
    , m_nFrameRate(0)
    , m_sAudioCodec("opus")
    , network_(CreateNetwork())
    , worker_(CreateWorker())
    , signaling_(CreateSignaling())
    , adm_(CreateADM(worker_.get()))
    , apm_(CreateAPM())
    , factory_(nullptr)
    , pc_(nullptr)
    , audioSource_(nullptr)
    , videoSource_(nullptr)
    , audioSender_(nullptr)
    , videoSender_(nullptr)
    , logSink_(std::make_unique<LogSink>())
    , logLevel_(LoggingSeverity::LS_NONE)
{
    obs_info("WebRTCStream ctor");
    rtc::LogMessage::LogThreads(true);
    LogLevel(LoggingSeverity::LS_VERBOSE);
    ResetStats();

    factory_ = CreatePCFactory(network_.get(), worker_.get(), signaling_.get(), adm_, apm_);
}


WebRTCStream::~WebRTCStream()
{
    obs_info("~WebRTCStream dtor");
    Stop(false);

    if (m_pOutput)
    {
        obs_output_release(m_pOutput);
        m_pOutput = nullptr;
    }

    if (m_pWsClient)
        m_pWsClient = nullptr;

    audioSender_    = nullptr;
    videoSender_    = nullptr;
    audioSource_    = nullptr;
    pc_             = nullptr;
    factory_        = nullptr;
    apm_            = nullptr;

    worker_->Invoke<void>(RTC_FROM_HERE, [this]() { adm_ = nullptr; });
    signaling_->Invoke<void>(RTC_FROM_HERE, [this]() { videoSource_ = nullptr; });

    if (!network_->IsCurrent())
        network_->Stop();
    if (!worker_->IsCurrent())
        worker_->Stop();
    if (!signaling_->IsCurrent())
        signaling_->Stop();

    network_ = nullptr;
    worker_ = nullptr;
    signaling_ = nullptr;

    rtc::LogMessage::RemoveLogToStream(logSink_.get());
}


bool WebRTCStream::Start()
{
    if (started_)
        Stop(false);
    started_ = true;
    stopping_ = false;

    ResetStats();

    if (!AuthenticateModel())
        return false;

    ConfigureStreamParameters();

    if (!CreatePeerConnection())
        return false;
    if (!AddTracks())
        return false;
    if (!OpenWebsocketConnection())
        return false;

    return true;
}


bool WebRTCStream::Stop(bool normal)
{
    obs_info("WebRTCStream::stop");
    bool ret = false;
    started_ = false;
    stopping_ = true;

    // Increase logging verbosity to log end of stream summary
    LogLevel(LoggingSeverity::LS_VERBOSE);

    if (audioSender_)
        audioSender_.release();
    if (videoSender_)
        videoSender_.release();

    if (pc_)
    {
        ret = true;
        auto old = pc_.release();
        old->Close();  // Close PeerConnection
        old = nullptr;
    }

    if (m_pWsClient)
        m_pWsClient->disconnect(normal);  // Close websocket connection

    obs_output_end_data_capture(m_pOutput);  // Stop main thread
    return ret;
}


void WebRTCStream::ResetStats()
{
    frame_id_               = 0;
    video_bitrate_bps_      = 0;
    total_bitrate_bps_      = 0;

    // Outbound RTP Stream Stats
    outbound_time_us_       = 0;
    packets_sent_           = 0;
    packets_rtx_            = 0;
    audio_bytes_sent_       = 0;
    video_bytes_sent_       = 0;
    video_bytes_rtx_        = 0;
    outbound_frames_sent_   = 0;
    outbound_fps_           = 0;
    total_pkt_send_delay_   = 0;

    video_bitrate_          = 0;
    avg_video_bitrate_      = 0;

    prev_video_bytes_       = 0;
    prev_frames_sent_       = 0;
    prev_timestamp_         = 0;

    // Media Stream Track Stats
    frame_width_            = 0;
    frame_height_           = 0;
    frames_sent_            = 0;
    huge_frames_sent_       = 0;
    frames_dropped_         = 0;
    frames_per_second_      = 0;

    // RTP Stream Stats
    nack_received_          = 0;
    pli_received_           = 0;
    sli_received_           = 0;

    // Remote Inbound RTP Stream Stats
    packets_lost_           = 0;
    rtt_                    = 0;
    jitter_                 = 0;
}


bool WebRTCStream::AuthenticateModel()
{
#ifdef MFC_API_CLIENT_ID
    auto lk = g_apiCtx.sharedLock();

    if (!g_apiCtx.IsLinked())
    {
        ui_MsgBox("Unable to start WebRTC Stream",
                  "You must first link to your MFC model account before starting a stream.",
                  1024,
                  (QWidget*)nullptr);
        return false;

        //g_apiCtx.Link();
        //QString qstr = QString::fromUtf8(g_apiCtx.linkUrl().c_str());
        //QDesktopServices::openUrl(QUrl(qstr));
    }

    if (!g_apiCtx.HaveCredentials())
        return false;

    obs_info("Credentials sucessfully retreived from MFC");

    m_sVideoCodec   = g_apiCtx.codec();
    m_sProtocol     = g_apiCtx.prot();
    m_sRegion       = g_apiCtx.region();
    m_sUsername     = g_apiCtx.username();
    m_sPwd          = g_apiCtx.pwd();
    m_fCamScore     = g_apiCtx.camscore();
    m_nSid          = g_apiCtx.sid();
    m_nUid          = g_apiCtx.uid();
    m_nRoomId       = g_apiCtx.room();
    m_sStreamKey    = g_apiCtx.streamkey();
    m_sVidCtx       = g_apiCtx.vidctx();
    m_sVideoServer  = g_apiCtx.videoserver();
#else
    auto lk         = g_ctx.sharedLock();

    m_sVideoCodec   = g_ctx.cfg.getString("codec");
    m_sProtocol     = g_ctx.cfg.getString("prot");
    m_sRegion       = g_ctx.cfg.getString("region");
    m_sUsername     = g_ctx.cfg.getString("username");
    m_sPwd          = g_ctx.cfg.getString("pwd");
    m_fCamScore     = g_ctx.cfg.getFloat("camscore");
    m_nSid          = g_ctx.cfg.getInt("sid");
    m_nUid          = g_ctx.cfg.getInt("uid");
    m_nRoomId       = g_ctx.cfg.getInt("room");
    m_sStreamKey    = g_ctx.cfg.getString("ctx");
    m_sVidCtx       = g_ctx.cfg.getString("vidctx");
    m_sVideoServer  = g_ctx.cfg.getString("videoserver");
#endif
    return true;
}


void WebRTCStream::ConfigureStreamParameters()
{
    obs_encoder_t* pAudioEncoder = obs_output_get_audio_encoder(m_pOutput, 0);
    obs_data_t* pAudioSettings = obs_encoder_get_settings(pAudioEncoder);
    m_nAudioBitrateKbps = (int)obs_data_get_int(pAudioSettings, "bitrate");
    obs_data_release(pAudioSettings);

    obs_encoder_t* pVideoEncoder = obs_output_get_video_encoder(m_pOutput);
    double fps = video_output_get_frame_rate(obs_get_video());
    m_nFrameRate = std::min((int)round(fps), 30);
    obs_data_t* pVideoSettings = obs_encoder_get_settings(pVideoEncoder);
    int videoBitrateKbps = (int)obs_data_get_int(pVideoSettings, "bitrate");
    obs_data_release(pVideoSettings);

    m_nWidth = (int)obs_output_get_width(m_pOutput);
    m_nHeight = (int)obs_output_get_height(m_pOutput);

#if MFC_LIMIT_WEBRTC_BITRATE_2500
    // Limiting bitrate until multiple webrtc renditions are supported
    m_nVideoBitrateKbps = std::min(videoBitrateKbps, 2500);
#else
    m_nVideoBitrateKbps = videoBitrateKbps;
#endif
    obs_info("\nOriginal resolution: %d x %d", m_nWidth, m_nHeight);
    SanitizeInputs::OptimalFrameSize(m_nVideoBitrateKbps, m_nWidth, m_nHeight);
    SanitizeInputs::OpusBitrate(m_nVideoBitrateKbps, m_nAudioBitrateKbps);
    SanitizeInputs::ConstrainBitrate(m_nHeight, m_nVideoBitrateKbps);
    m_nVideoBitrateKbps = roundUp(m_nVideoBitrateKbps, m_nFrameRate);
    obs_info("Adjusted resolution: %d x %d\n", m_nWidth, m_nHeight);

    video_bitrate_bps_ = m_nVideoBitrateKbps * 1000;
    total_bitrate_bps_ = (m_nVideoBitrateKbps + m_nAudioBitrateKbps) * 1000;

    audio_convert_info aci{};
    aci.format = AUDIO_FORMAT_16BIT;
    aci.samples_per_sec = 48000;
    aci.speakers = SPEAKERS_STEREO;
    obs_output_set_audio_conversion(m_pOutput, &aci);

    video_scale_info vsi{};
    vsi.format = VIDEO_FORMAT_NV12;
    vsi.range = VIDEO_RANGE_PARTIAL;
    vsi.colorspace = VIDEO_CS_709;
    vsi.width = (uint32_t)m_nWidth;
    vsi.height = (uint32_t)m_nHeight;
    obs_output_set_preferred_size(m_pOutput, vsi.width, vsi.height);
    obs_output_set_video_conversion(m_pOutput, &vsi);

    obs_output_set_media(m_pOutput, obs_get_video(), obs_get_audio());
}


bool WebRTCStream::CreatePeerConnection()
{
    if (!factory_)
    {
        obs_error("Error initializing PeerConnection Factory");
        return false;
    }

    PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = SdpSemantics::kUnifiedPlan;
    config.set_cpu_adaptation(false);
    config.set_prerenderer_smoothing(false);

    PeerConnectionDependencies dependencies(this);

    pc_ = factory_->CreatePeerConnection(config, std::move(dependencies));
    if (!pc_)
    {
        obs_error("Error creating PeerConnection");
        return false;
    }
    obs_info("PeerConnection CREATED\n");

    return true;
}


bool WebRTCStream::AddTracks()
{
    const string stream_id = "obs";

    cricket::AudioOptions options;
    options.echo_cancellation       = false;
    options.auto_gain_control       = false;
    options.noise_suppression       = false;
    options.highpass_filter         = false;
    options.typing_detection        = false;
    options.residual_echo_detector  = false;
    options.stereo_swapping         = false;
    options.experimental_agc        = false;
    options.experimental_ns         = false;

    audioSource_ = factory_->CreateAudioSource(options);
    audioTrack_ = factory_->CreateAudioTrack("audio", audioSource_);
    auto audio_result_or_error = pc_->AddTrack(audioTrack_, {stream_id});
    if (!audio_result_or_error.ok())
    {
        auto error = audio_result_or_error.MoveError();
        obs_warn("Error adding audio track to PeerConnection: %s", error.message());
        return false;
    }
    audioSender_ = audio_result_or_error.MoveValue();
    obs_info("Added audio track to PeerConnection\n");

    videoSource_ =
        signaling_->Invoke<scoped_refptr<VideoTrackSource>>(
            RTC_FROM_HERE, []() { return VideoTrackSource::Create(); });
    videoTrack_ = factory_->CreateVideoTrack("video", videoSource_);
    auto video_result_or_error = pc_->AddTrack(videoTrack_, {stream_id});
    if (!video_result_or_error.ok())
    {
        auto error = video_result_or_error.MoveError();
        obs_warn("Error adding video track to PeerConnection: %s", error.message());
        return false;
    }
    videoSender_ = video_result_or_error.MoveValue();
    obs_info("Added video track to PeerConnection\n");

    return true;
}


bool WebRTCStream::OpenWebsocketConnection()
{
    string sStreamName = "ext_x_" + std::to_string(m_nUid) + ".f4v";
    string sWsUrl      = "wss://" + m_sVideoServer + ".myfreecams.com/webrtc-session.json";
    m_sProtocol.clear();

    obs_info("Video codec:         %s\n", m_sVideoCodec.c_str());
    obs_info("Video bitrate:       %d", m_nVideoBitrateKbps);
    obs_info("Audio bitrate:       %d\n", m_nAudioBitrateKbps);

    m_pWsClient = CreateWebsocketClient();
    if (!m_pWsClient)
    {
        obs_warn("Error creating Websocket client");
        Stop(false);  // Close PeerConnection
        obs_output_set_last_error(m_pOutput, "Error creating Websocket client\n\n");
        obs_output_signal_stop(m_pOutput, OBS_OUTPUT_CONNECT_FAILED);  // Stop main thread
        return false;
    }

    obs_info("region: %s\nurl: %s", m_sRegion.c_str(), sWsUrl.c_str());
    obs_info("username: %s, uid: %d, sid: %d", m_sUsername.c_str(), m_nUid, m_nSid);
    obs_info("stream name: %s\npassword: %s", sStreamName.c_str(), m_sPwd.c_str());
    obs_info("vidctx: %s\nstream key: %s", m_sVidCtx.c_str(), m_sStreamKey.c_str());

    if (!m_pWsClient->connect(this, sWsUrl, sStreamName, m_sStreamKey, m_sPwd, m_sVidCtx, m_nSid,
                              m_nUid, m_nRoomId, m_nWidth, m_nHeight, m_nFrameRate, m_fCamScore))
    {
        obs_error("Error connecting to server");
        Stop(false);
        obs_output_set_last_error(m_pOutput, "Connection Failed\n\n");
        obs_output_signal_stop(m_pOutput, OBS_OUTPUT_CONNECT_FAILED);
        return false;
    }

    return true;
}


void WebRTCStream::onConnected()
{
    obs_info("SETTING LOCAL DESCRIPTION\n\n");
    pc_->SetLocalDescription(this);
}


void WebRTCStream::OnSetLocalDescriptionComplete(RTCError error)
{
    if (error.ok())
    {
        obs_info("\nLocal Description set\n");
        SendOffer(pc_->local_description());
    }
    else
    {
        obs_error("\nError setting Local Description: %s\n", error.message());
        // Close must be carried out on a separate thread in order to avoid deadlock
        auto thread = std::thread([=]()
        {
            Stop(false);
            obs_output_set_last_error(m_pOutput, "Error setting Remote Description\n\n");
            obs_output_signal_stop(m_pOutput, OBS_OUTPUT_ERROR);
        });
        thread.detach();
    }
}


void WebRTCStream::SendOffer(const SessionDescriptionInterface* desc)
{
    string sdp;
    desc->ToString(&sdp);
    obs_info("OFFER:\n\n%s\n", sdp.c_str());

    // If codec setting is Automatic
    if (m_sVideoCodec.empty())
        m_sAudioCodec.clear();

    vector<int> audioPayloads;
    vector<int> videoPayloads;

    SDPUtil::ForcePayload(sdp, audioPayloads, videoPayloads, m_sAudioCodec, m_sVideoCodec, 0, "42e01f", 0);
    SDPUtil::ConstrainVideoBitrate(sdp, m_nVideoBitrateKbps, m_nFrameRate);
    SDPUtil::ConstrainAudioBitrateAS(sdp, m_nAudioBitrateKbps, false);
    SDPUtil::EnableStereo(sdp);
    SDPUtil::RemoveRtcpFb(sdp, "transport-cc");
    SDPUtil::RemoveLinesContaining(sdp, "transport-wide-cc");

    obs_info("Sending OFFER (SDP) to remote peer:\n\n%s", sdp.c_str());
    if (m_pWsClient->sendSdp(sdp, m_sVideoCodec))
        obs_info("Offer successfully sent to remote peer");
    else
        obs_warn("Failed to send offer to remote peer!");
}


void WebRTCStream::SetBitrate()
{
    for (auto& sender : pc_->GetSenders())
    {
        auto params = sender->GetParameters();
        if (sender->media_type() == cricket::MEDIA_TYPE_VIDEO)
        {
            if (!params.encodings.empty())
            {
                params.encodings[0].max_bitrate_bps = video_bitrate_bps_;
                //params.encodings[0].min_bitrate_bps = absl::optional<int>(video_bitrate_bps_ * 2 / 3);
                params.encodings[0].max_framerate   = (double)m_nFrameRate;
                obs_info("Setting video RTP sender min/max bitrate");
                RTCError ret = sender->SetParameters(params);
                if (ret.ok())
                {
                    obs_info("max bitrate:   %d", video_bitrate_bps_);
                    //obs_info("min bitrate:   %d", video_bitrate_bps_);
                    obs_info("max framerate: %d", m_nFrameRate);
                }
                else obs_warn("Failed to set RTP parameters: %s", ret.message());
            }
        }
    }
}


void WebRTCStream::onAnswer(const string& sdp)
{
    obs_info("ANSWER:\n\n%s\n", sdp.c_str());

    SdpParseError error;
    auto answer = CreateSessionDescription(SdpType::kAnswer, sdp, &error);
    if (!answer)
    {
        obs_warn("Error parsing received SDP: %s\n", error.description.c_str());
        return;
    }

    obs_info("\nSETTING REMOTE DESCRIPTION\n\n%s", sdp.c_str());
    pc_->SetRemoteDescription(std::move(answer), this);
}


void WebRTCStream::OnIceCandidate(const IceCandidateInterface* candidate)
{
    string str;
    candidate->ToString(&str);

    const string mid = candidate->sdp_mid();
    const int mline_index = candidate->sdp_mline_index();

    // Send candidate to remote peer
    m_pWsClient->trickle(str, mid, mline_index, false);
}


void WebRTCStream::onRemoteIceCandidate(const string& candidate, const string& mid, int index)
{
    if (candidate.empty() || index == -1)
    {
        obs_info("ICE COMPLETE\n");
        return;
    }
    obs_info("\n**** WebRTCStream::onRemoteIceCandidate ****");

    string s = candidate;
    s.erase(remove(s.begin(), s.end(), '\"'), s.end());

    if (!m_sProtocol.empty() && !SDPUtil::IsProtocol(s, m_sProtocol))
    {
        obs_info("\nIgnoring remote %s\n", s.c_str());
        return;
    }

    string proto, ip;
    int port = 0;
    SDPUtil::ParseIceCandidate(candidate, proto, ip, port);
    string iceIpPortProtocol = ip + ":" + std::to_string(port) + " " + proto;

    SdpParseError error;
    cricket::Candidate c;
    webrtc::SdpDeserializeCandidate(mid, s, &c, &error);

    string site = m_sRegion;
    MFCEdgeIngest::SiteIpPort edge;
    if (m_sRegion.empty())
    {
        edge = MFCEdgeIngest::WebrtcTcpIpPort(m_sVideoServer);
        site = edge.site;
    }
    else if (!SDPUtil::CaseInsStringCompare(m_sRegion, "TUK")
             && !SDPUtil::CaseInsStringCompare(m_sRegion, "ORD"))
    {
        edge = MFCEdgeIngest::WebrtcTcpIpPort(m_sVideoServer, m_sRegion);
        site = edge.site;
    }
    if (!edge.tcpIp.empty() && edge.port > 0
        && !SDPUtil::CaseInsStringCompare(site, "TUK")
        && !SDPUtil::CaseInsStringCompare(site, "ORD"))
    {
        if (SDPUtil::IsProtocol(s, "TCP"))
        {
            c.set_address(rtc::SocketAddress(edge.tcpIp, edge.port));
            string originalIceIpPort = iceIpPortProtocol;
            iceIpPortProtocol = edge.tcpIp + ":" + std::to_string(edge.port) + " " + proto + " (" + site + ")";
            obs_info("\nModifying remote candidate - old: %s, new: %s",
                     originalIceIpPort.c_str(), iceIpPortProtocol.c_str());
        }
        else
        {
            obs_info("\nIgnoring remote %s\n", s.c_str());
            return;
        }
    }
    obs_info("\nAdding remote candidate: %s\n", iceIpPortProtocol.c_str());

    auto iceCandidate = CreateIceCandidate(mid, index, c);
    if (!iceCandidate)
    {
        obs_warn("\nError parsing received candidate: %s\n", error.description.c_str());
        return;
    }

    std::function<void (RTCError)> onAddCandidateError = [](RTCError error)
    {
        if (!error.ok())
            blog(200, "\nError adding ice candidate: %s\n", error.message());
    };

    pc_->AddIceCandidate(std::move(iceCandidate), onAddCandidateError);
}


void WebRTCStream::OnSetRemoteDescriptionComplete(RTCError error)
{
    if (!error.ok())
    {
        obs_error("\nError setting Remote Description: %s\n", error.message());
        // Close must be carried out on a separate thread in order to avoid deadlock
        auto thread = std::thread([=]()
        {
            Stop(false);
            obs_output_set_last_error(m_pOutput, "Error setting Remote Description\n\n");
            obs_output_signal_stop(m_pOutput, OBS_OUTPUT_ERROR);
        });
        thread.detach();
    }

    obs_info("\nRemote Description set\n");
}


void WebRTCStream::onReadyToStartBroadcast()
{
    ConfigureAPM(apm_);

#if WEBRTCSTREAM_USE_BITRATE_SETTINGS
    BitrateSettings settings;
    settings.max_bitrate_bps    = video_bitrate_bps_;
    settings.min_bitrate_bps    = video_bitrate_bps_ * 3 / 5;
    settings.start_bitrate_bps  = video_bitrate_bps_ * 2 / 3;
#endif

#if WEBRTCSTREAM_MODIFY_SENDER_PARAMETERS
    SetBitrate();
#endif

    obs_info("\nBeginning data capture...\n");
    if (!obs_output_begin_data_capture(m_pOutput, 0))
        obs_error("\nError initiating OBS data capture\n");
}


void WebRTCStream::onAudioFrame(audio_data* frame)
{
    if (!frame || !adm_)
        return;

    worker_->Invoke<void>(
        RTC_FROM_HERE, [this,frame]() { adm_->onIncomingData(frame->data[0], frame->frames); });
}


void WebRTCStream::onVideoFrame(video_data* frame)
{
    if (!frame || !videoSource_)
        return;

    videoSource_->onIncomingData(frame->data[0], frame->linesize[0],
                                 frame->data[1], frame->linesize[1],
                                 (int64_t)frame->timestamp, ++frame_id_,
                                 m_nWidth, m_nHeight,
                                 kVideoRotation_0);
}


void WebRTCStream::OnIceConnectionChange(PeerConnectionInterface::IceConnectionState state)
{
    obs_info("WebRTCStream::OnIceConnectionChange [%u]", state);

    switch (state)
    {
    case PeerConnectionInterface::IceConnectionState::kIceConnectionFailed:
    {
        obs_error("Ice Connection Failed");
        // Close must be carried out on a separate thread in order to avoid deadlock
        auto thread = std::thread([=]()
        {
            obs_output_set_last_error(m_pOutput, "Ice Connection Failed\n\n");
            obs_output_signal_stop(m_pOutput, OBS_OUTPUT_ERROR);
        });
        thread.detach();
        break;
    }
    default:
        break;
    }
}


void WebRTCStream::OnConnectionChange(PeerConnectionInterface::PeerConnectionState state)
{
    obs_info("WebRTCStream::OnConnectionChange [%u]", state);

    switch (state)
    {
    case PeerConnectionInterface::PeerConnectionState::kFailed:
    {
        obs_error("Connection Failed");
        // Close must be carried out on a separate thread in order to avoid deadlock
        auto thread = std::thread([=]()
        {
            obs_output_set_last_error(m_pOutput, "Connection Failed\n\n");
            obs_output_signal_stop(m_pOutput, OBS_OUTPUT_ERROR);
        });
        thread.detach();
        break;
    }
    default:
        break;
    }
}


void WebRTCStream::GetStats()
{
    if (!pc_)
        return;

    pc_->GetStats(this);
}


void WebRTCStream::OnStatsDelivered(const scoped_refptr<const RTCStatsReport>& report)
{
    MutexLock lock(&mutex_);

    if (report)
    {
        auto remote_inbound_stream_stats(report->GetStatsOfType<RTCRemoteInboundRtpStreamStats>());
        auto outbound_rtp_stream_stats(report->GetStatsOfType<RTCOutboundRTPStreamStats>());
        auto media_stream_track_stats(report->GetStatsOfType<RTCMediaStreamTrackStats>());

        for (const auto& stat : remote_inbound_stream_stats)
        {
            if (stat->kind.ValueToString() == "video")
            {
                packets_lost_   = *stat->packets_lost;
                rtt_            = (int)round(*stat->round_trip_time * 1000);
                jitter_         = *stat->jitter * 1000;
            }
        }

        double outbound_delta_t = 0.0;
        double prev_outbound_delta_t = 0.0;
        for (const auto& stat : outbound_rtp_stream_stats)
        {
            if (stat->kind.ValueToString() == "audio")
            {
                audio_bytes_sent_ = *stat->bytes_sent;
            }
            else if (stat->kind.ValueToString() == "video")
            {
                nack_received_  = *stat->nack_count;
                pli_received_   = *stat->pli_count;
                sli_received_   = *stat->sli_count;

                int64_t delta_t_us = 0;
                int64_t prev_delta_t_us = 0;
                if (outbound_time_us_ > 0)
                {
                    delta_t_us = stat->timestamp_us() - outbound_time_us_;
                    outbound_delta_t = delta_t_us / (double)rtc::kNumMicrosecsPerSec;
                }
                outbound_time_us_ = stat->timestamp_us();

                if (prev_timestamp_ > 0)
                {
                    prev_delta_t_us = outbound_time_us_ - prev_timestamp_;
                    prev_outbound_delta_t = prev_delta_t_us / (double)rtc::kNumMicrosecsPerSec;
                }
                else
                {
                    prev_timestamp_ = outbound_time_us_;
                }

                uint64_t temp           = *stat->bytes_sent;
                video_bytes_rtx_        = *stat->retransmitted_bytes_sent;
                packets_sent_           = *stat->packets_sent;
                packets_rtx_            = *stat->retransmitted_packets_sent;
                outbound_frames_sent_   = *stat->frames_sent;
                outbound_fps_           = *stat->frames_per_second;
                total_pkt_send_delay_   = *stat->total_packet_send_delay;

                if (delta_t_us > 0)
                    video_bitrate_ = (uint32_t)((temp - video_bytes_sent_) * 8 / (delta_t_us / 1000));
                video_bytes_sent_ = temp;
            }
        }

        //double media_stream_delta_t = 0.0;
        for (const auto& stat : media_stream_track_stats)
        {
            if (stat->kind.ValueToString() == "video")
            {
                frame_width_            = *stat->frame_width;
                frame_height_           = *stat->frame_height;
                frames_sent_            = *stat->frames_sent;
                huge_frames_sent_       = *stat->huge_frames_sent;
                frames_dropped_         = *stat->frames_dropped;
                //frames_per_second_      = *stat->frames_per_second;
            }
        }

        // output every 15 seconds
        if (prev_outbound_delta_t > 14)
        {
            std::ostringstream oss;
            std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            auto loc_time = std::put_time(std::localtime(&t), "%FT%T%z");
            oss << loc_time;

            avg_video_bitrate_ = ((video_bytes_sent_ - prev_video_bytes_) * 8 / (uint32_t)(prev_outbound_delta_t * 1000));

            obs_info("date + time:       %s",       oss.str().c_str());
            obs_info("timestamp:         %lld",     outbound_time_us_);
            obs_info("frame size:        %u x %u",  frame_width_, frame_height_);
            obs_info("fps out:           %f",       outbound_fps_);
            obs_info("video bitrate:     %u",       video_bitrate_);
            obs_info("avg video bitrate: %u",       avg_video_bitrate_);
            obs_info("frames sent:       %u",       frames_sent_);
            obs_info("huge frames sent:  %u",       huge_frames_sent_);
            obs_info("packets sent:      %u",       packets_sent_);
            obs_info("packets rtx:       %llu",     packets_rtx_);
            obs_info("video bytes sent:  %llu",     video_bytes_sent_);
            obs_info("video bytes rtx:   %llu",     video_bytes_rtx_);
            obs_info("round trip time:   %d",       rtt_);
            obs_info("jitter:            %f",       jitter_);
            obs_info("frames dropped:    %u",       frames_dropped_);
            obs_info("packets lost:      %d",       packets_lost_);
            obs_info("PLI received:      %u",       pli_received_);
            obs_info("SLI received:      %u",       sli_received_);
            obs_info("NACK received:     %u",       nack_received_);
            obs_info("packet send delay: %d",       (int)round(total_pkt_send_delay_ / packets_sent_ * 1000));

            prev_video_bytes_   = video_bytes_sent_;
            prev_timestamp_     = outbound_time_us_;

            // Decrease logging verbosity
            if (!stopping_)
                LogLevel(LoggingSeverity::LS_INFO);
        }
    }
}


void WebRTCStream::onAuthFailure()
{
    obs_info(__FUNCTION__);
    Stop(false);
    obs_output_set_last_error(m_pOutput, "Authentication Failed\n\n");
    obs_output_signal_stop(m_pOutput, OBS_OUTPUT_INVALID_STREAM);
}


void WebRTCStream::onConnectError()
{
    obs_error(__FUNCTION__);
    Stop(false);
    obs_output_set_last_error(m_pOutput, "Websocket Connection Error\n\n");
    obs_output_signal_stop(m_pOutput, OBS_OUTPUT_ERROR);
}


void WebRTCStream::onDisconnected()
{
    obs_info(__FUNCTION__);

    if (close_async_.joinable())
        close_async_.join();

    // Close must be carried out on a separate thread in order to avoid deadlock
    close_async_ = std::thread([&]()
    {
        Stop(false);
        obs_output_set_last_error(m_pOutput, "Websocket Disconnected\n\n");
        obs_output_signal_stop(m_pOutput, OBS_OUTPUT_ERROR);
    });
}


#if 0
void WebRTCStream::OnFailure(RTCError error)
{
    obs_info("__FUNCTION__: %s", error.message());
    Stop(false);
    obs_output_set_last_error(m_pOutput, "Create/Set Session Description Failed\n\n");
    obs_output_signal_stop(m_pOutput, OBS_OUTPUT_ERROR);
}
#endif


void WebRTCStream::LogLevel(rtc::LoggingSeverity logLevel)
{
    if (logLevel_ == logLevel) return;
    if (logLevel_ != rtc::LoggingSeverity::LS_NONE)
        rtc::LogMessage::RemoveLogToStream(logSink_.get());
    rtc::LogMessage::AddLogToStream(logSink_.get(), logLevel);
    logLevel_ = logLevel;
}
