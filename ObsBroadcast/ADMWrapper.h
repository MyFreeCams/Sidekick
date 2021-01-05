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

#ifndef ADM_WRAPPER_H_
#define ADM_WRAPPER_H_

#include "api/scoped_refptr.h"
#include "modules/audio_device/include/audio_device.h"
#include "rtc_base/synchronization/mutex.h"

#include <cstdint>
#include <vector>

using webrtc::AudioDeviceModule;
using webrtc::AudioTransport;
using webrtc::Mutex;
using webrtc::kAdmMaxDeviceNameSize;
using webrtc::kAdmMaxGuidSize;

class ADMWrapper : public AudioDeviceModule
{
public:
    ADMWrapper();
    ~ADMWrapper() override;

    static rtc::scoped_refptr<ADMWrapper> Create();

    virtual void onIncomingData(uint8_t* data, size_t samplesPerChannel);

    // Main initialization and termination
    int32_t Init() override;
    int32_t Terminate() override;
    bool Initialized() const override { return initialized_; }

    // Retrieve the currently utilized audio layer
    int32_t ActiveAudioLayer(AudioLayer* audioLayer) const override;

    // Full-duplex transportation of PCM audio
    int32_t RegisterAudioCallback(AudioTransport* audioCallback) override;

    // Device enumeration
    int16_t PlayoutDevices() override { return 0; }
    int16_t RecordingDevices() override { return 1; }
    int32_t PlayoutDeviceName(
        uint16_t index, char name[kAdmMaxDeviceNameSize], char guid[kAdmMaxGuidSize]) override { return -1; }
    int32_t RecordingDeviceName(
        uint16_t index, char name[kAdmMaxDeviceNameSize], char guid[kAdmMaxGuidSize]) override { return -1; }

    // Device selection
    int32_t SetPlayoutDevice(uint16_t index) override { return -1; }
    int32_t SetPlayoutDevice(WindowsDeviceType device) override { return -1; }
    int32_t SetRecordingDevice(uint16_t index) override { return 0; }
    int32_t SetRecordingDevice(WindowsDeviceType device) override { return 0; }

    // Audio transport initialization
    int32_t PlayoutIsAvailable(bool* available) override
    {
        *available = false;
        return 0;
    }
    int32_t InitPlayout() override { return -1; }
    bool PlayoutIsInitialized() const override { return false; }
    int32_t RecordingIsAvailable(bool* available) override
    {
        *available = true;
        return 0;
    }
    int32_t InitRecording() override { return 0; }
    bool RecordingIsInitialized() const override { return true; }

    // Audio transport control
    int32_t StartPlayout() override { return -1; }
    int32_t StopPlayout() override { return -1; }
    bool Playing() const override { return false; }
    int32_t StartRecording() override { return 0; }
    int32_t StopRecording() override { return 0; }
    bool Recording() const override { return true; }

    // Audio mixer initialization
    int32_t InitSpeaker() override { return -1; }
    bool SpeakerIsInitialized() const override { return false; }
    int32_t InitMicrophone() override { return -1; }
    bool MicrophoneIsInitialized() const override { return false; }

    // Speaker volume controls
    int32_t SpeakerVolumeIsAvailable(bool* available) override
    {
        *available = false;
        return 0;
    }
    int32_t SetSpeakerVolume(uint32_t volume) override { return -1; }
    int32_t SpeakerVolume(uint32_t* volume) const override { return -1; }
    int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override { return -1; }
    int32_t MinSpeakerVolume(uint32_t* minVolume) const override { return -1; }

    // Microphone volume controls
    int32_t MicrophoneVolumeIsAvailable(bool* available) override
    {
        *available = false;
        return 0;
    }
    int32_t SetMicrophoneVolume(uint32_t volume) override { return -1; }
    int32_t MicrophoneVolume(uint32_t* volume) const override { return -1; }
    int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override { return -1; }
    int32_t MinMicrophoneVolume(uint32_t* minVolume) const override { return -1; }

    // Speaker mute control
    int32_t SpeakerMuteIsAvailable(bool* available) override
    {
        *available = false;
        return 0;
    }
    int32_t SetSpeakerMute(bool enable) override { return -1; }
    int32_t SpeakerMute(bool* enabled) const override
    {
        *enabled = false;
        return 0;
    }

    // Microphone mute control
    int32_t MicrophoneMuteIsAvailable(bool* available) override
    {
        *available = false;
        return 0;
    }
    int32_t SetMicrophoneMute(bool enable) override { return -1; }
    int32_t MicrophoneMute(bool* enabled) const override
    {
        *enabled = false;
        return 0;
    }

    // Stereo support
    int32_t StereoPlayoutIsAvailable(bool* available) const override
    {
        *available = true;
        return 0;
    }
    int32_t SetStereoPlayout(bool enable) override { return 0; }
    int32_t StereoPlayout(bool* enabled) const override
    {
        *enabled = true;
        return 0;
    }
    int32_t StereoRecordingIsAvailable(bool* available) const override
    {
        *available = true;
        return 0;
    }
    int32_t SetStereoRecording(bool enable) override { return 0; }
    int32_t StereoRecording(bool* enabled) const override
    {
        *enabled = true;
        return 0;
    }

    // Playout delay
    int32_t PlayoutDelay(uint16_t* delayMS) const override
    {
        *delayMS = 0;
        return 0;
    }

    // Built-in audio effects. Only supported on Android.
    bool BuiltInAECIsAvailable() const override { return false; }
    int32_t EnableBuiltInAEC(bool enable) override { return -1; }
    bool BuiltInAGCIsAvailable() const override { return false; }
    int32_t EnableBuiltInAGC(bool enable) override { return -1; }
    bool BuiltInNSIsAvailable() const override { return false; }
    int32_t EnableBuiltInNS(bool enable) override { return -1; }

private:
    AudioTransport*         audio_transport_;

    Mutex                   mutex_;

    std::vector<uint8_t>    buffer_;
    size_t                  buffer_idx_;

    bool                    initialized_;
};

#endif  // ADM_WRAPPER_H_
