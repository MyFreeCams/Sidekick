#include "VideoSendStream.h"

#include <utility>

#include "absl/algorithm/container.h"
#include "media/base/rtp_utils.h"
#include "rtc_base/checks.h"
#include "rtc_base/gunit.h"


FakeVideoSendStream::FakeVideoSendStream(
        webrtc::VideoSendStream::Config config,
        webrtc::VideoEncoderConfig encoder_config)
        : sending_(false),
            config_(std::move(config)),
            codec_settings_set_(false),
            resolution_scaling_enabled_(false),
            framerate_scaling_enabled_(false),
            source_(nullptr),
            num_swapped_frames_(0)
{
    RTC_DCHECK(config.encoder_settings.encoder_factory != nullptr);
    RTC_DCHECK(config.encoder_settings.bitrate_allocator_factory != nullptr);
    ReconfigureVideoEncoder(std::move(encoder_config));
}


FakeVideoSendStream::~FakeVideoSendStream()
{
    if (source_)
        source_->RemoveSink(this);
}


const webrtc::VideoSendStream::Config& FakeVideoSendStream::GetConfig() const
{
    return config_;
}

const webrtc::VideoEncoderConfig& FakeVideoSendStream::GetEncoderConfig() const
{
    return encoder_config_;
}

const std::vector<webrtc::VideoStream>& FakeVideoSendStream::GetVideoStreams() const
{
    return video_streams_;
}

bool FakeVideoSendStream::IsSending() const
{
    return sending_;
}


bool FakeVideoSendStream::GetVp8Settings(webrtc::VideoCodecVP8* settings) const
{
    if (!codec_settings_set_) {
        return false;
    }
    *settings = codec_specific_settings_.vp8;
    return true;
}


bool FakeVideoSendStream::GetVp9Settings(webrtc::VideoCodecVP9* settings) const
{
    if (!codec_settings_set_) {
        return false;
    }
    *settings = codec_specific_settings_.vp9;
    return true;
}


bool FakeVideoSendStream::GetH264Settings(webrtc::VideoCodecH264* settings) const
{
    if (!codec_settings_set_) {
        return false;
    }
    *settings = codec_specific_settings_.h264;
    return true;
}


int FakeVideoSendStream::GetNumberOfSwappedFrames() const
{
    return num_swapped_frames_;
}

int FakeVideoSendStream::GetLastWidth() const
{
    return last_frame_->width();
}

int FakeVideoSendStream::GetLastHeight() const
{
    return last_frame_->height();
}

int64_t FakeVideoSendStream::GetLastTimestamp() const
{
    RTC_DCHECK(last_frame_->ntp_time_ms() == 0);
    return last_frame_->render_time_ms();
}

void FakeVideoSendStream::OnFrame(const webrtc::VideoFrame& frame)
{
    ++num_swapped_frames_;
    if (!last_frame_ || frame.width() != last_frame_->width() ||
            frame.height() != last_frame_->height() ||
            frame.rotation() != last_frame_->rotation()) {
        video_streams_ = encoder_config_.video_stream_factory->CreateEncoderStreams(
                frame.width(), frame.height(), encoder_config_);
    }
    last_frame_ = frame;
}


void FakeVideoSendStream::SetStats(const webrtc::VideoSendStream::Stats& stats)
{
    stats_ = stats;
}

webrtc::VideoSendStream::Stats FakeVideoSendStream::GetStats()
{
    return stats_;
}


void FakeVideoSendStream::ReconfigureVideoEncoder(webrtc::VideoEncoderConfig config)
{
    int width, height;
    if (last_frame_)
    {
        width = last_frame_->width();
        height = last_frame_->height();
    }
    else
    {
        width = height = 0;
    }
    video_streams_ = config.video_stream_factory->CreateEncoderStreams(width, height, config);
    if (config.encoder_specific_settings != NULL)
    {
        const unsigned char num_temporal_layers = static_cast<unsigned char>(
                video_streams_.back().num_temporal_layers.value_or(1));
        if (config_.rtp.payload_name == "VP8")
        {
            config.encoder_specific_settings->FillVideoCodecVp8(&codec_specific_settings_.vp8);
            if (!video_streams_.empty())
            {
                codec_specific_settings_.vp8.numberOfTemporalLayers = num_temporal_layers;
            }
        }
        else if (config_.rtp.payload_name == "VP9")
        {
            config.encoder_specific_settings->FillVideoCodecVp9(&codec_specific_settings_.vp9);
            if (!video_streams_.empty())
            {
                codec_specific_settings_.vp9.numberOfTemporalLayers = num_temporal_layers;
            }
        }
        else if (config_.rtp.payload_name == "H264")
        {
            config.encoder_specific_settings->FillVideoCodecH264(&codec_specific_settings_.h264);
            codec_specific_settings_.h264.numberOfTemporalLayers = num_temporal_layers;
        }
        else
        {
            ADD_FAILURE() << "Unsupported encoder payload: "
                          << config_.rtp.payload_name;
        }
    }
    codec_settings_set_ = config.encoder_specific_settings != NULL;
    encoder_config_ = std::move(config);
    ++num_encoder_reconfigurations_;
}


void FakeVideoSendStream::UpdateActiveSimulcastLayers(const std::vector<bool> active_layers)
{
    sending_ = false;
    for (const bool active_layer : active_layers)
    {
        if (active_layer)
        {
            sending_ = true;
            break;
        }
    }
}


void FakeVideoSendStream::Start()
{
    sending_ = true;
}

void FakeVideoSendStream::Stop()
{
    sending_ = false;
}

//void FakeVideoSendStream::AddAdaptationResource(rtc::scoped_refptr<webrtc::Resource> resource) {}


std::vector<rtc::scoped_refptr<webrtc::Resource>> FakeVideoSendStream::GetAdaptationResources()
{
    return {};
}


void FakeVideoSendStream::SetSource(rtc::VideoSourceInterface<webrtc::VideoFrame>* source,
                                    const webrtc::DegradationPreference& degradation_preference)
{
    if (source_)
        source_->RemoveSink(this);
    source_ = source;

    switch (degradation_preference)
    {
    case webrtc::DegradationPreference::MAINTAIN_FRAMERATE:
        resolution_scaling_enabled_ = true;
        framerate_scaling_enabled_ = false;
        break;
    case webrtc::DegradationPreference::MAINTAIN_RESOLUTION:
        resolution_scaling_enabled_ = false;
        framerate_scaling_enabled_ = true;
        break;
    case webrtc::DegradationPreference::BALANCED:
        resolution_scaling_enabled_ = true;
        framerate_scaling_enabled_ = true;
        break;
    case webrtc::DegradationPreference::DISABLED:
        resolution_scaling_enabled_ = false;
        framerate_scaling_enabled_ = false;
        break;
    }

    if (source)
        source->AddOrUpdateSink(this, resolution_scaling_enabled_  ? sink_wants_ : rtc::VideoSinkWants());
}


void FakeVideoSendStream::InjectVideoSinkWants(const rtc::VideoSinkWants& wants)
{
    sink_wants_ = wants;
    source_->AddOrUpdateSink(this, wants);
}
