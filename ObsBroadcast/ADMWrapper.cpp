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

#include "ADMWrapper.h"

#include "rtc_base/ref_counted_object.h"

static const size_t sampleRate = 48000;
static const size_t bitsPerSample = 16;
static const size_t nChannels = 2;

static const size_t nBytesPerSample = bitsPerSample / 8;  // 2
static const size_t nBytesPerFrame = nBytesPerSample * nChannels;  // 4

static const size_t bufferDurationMs = 10;
static const size_t buffersPerSecond = 1000 / bufferDurationMs;  // 100

static const size_t framesPer10msBuffer = sampleRate / buffersPerSecond;  // 480
static const size_t bytesPer10msBuffer = nBytesPerFrame * framesPer10msBuffer;  // 1920


ADMWrapper::ADMWrapper()
    : audio_transport_(nullptr)
    , buffer_(bytesPer10msBuffer)
    , buffer_idx_(0)
    , initialized_(false)
{}


ADMWrapper::~ADMWrapper()
{
    audio_transport_ = nullptr;
}


// static
rtc::scoped_refptr<ADMWrapper> ADMWrapper::Create()
{
    rtc::scoped_refptr<ADMWrapper> audio_device(new rtc::RefCountedObject<ADMWrapper>());
    audio_device->Init();
    return audio_device;
}


void ADMWrapper::onIncomingData(uint8_t* data, size_t samplesPerChannel)
{
    mutex_.Lock();
    if (!audio_transport_)
        return;
    mutex_.Unlock();

    size_t remainingData = samplesPerChannel * nBytesPerSample * nChannels;
    size_t remainingBuffer = buffer_.size() - buffer_idx_;
    size_t bytesToCopy = std::min(remainingData, remainingBuffer);

    while (bytesToCopy)
    {
        memcpy(&buffer_[buffer_idx_], data, bytesToCopy);

        data            += bytesToCopy;
        buffer_idx_     += bytesToCopy;
        remainingData   -= bytesToCopy;
        remainingBuffer -= bytesToCopy;

        if (remainingBuffer == 0)
        {
            uint32_t micLevel;
            audio_transport_->RecordedDataIsAvailable(&buffer_[0],
                                                      framesPer10msBuffer,
                                                      nBytesPerSample * nChannels,
                                                      nChannels,
                                                      sampleRate,
                                                      0, 0, 0, false, micLevel);
            buffer_idx_ = 0;
            remainingBuffer = buffer_.size();
        }

        bytesToCopy = std::min(remainingData, remainingBuffer);
    }
}


int32_t ADMWrapper::Init()
{
    if (initialized_)
        return 0;

    initialized_ = true;
    return 0;
}


int32_t ADMWrapper::Terminate()
{
    if (!initialized_)
        return 0;

    initialized_ = false;
    return 0;
}


int32_t ADMWrapper::ActiveAudioLayer(AudioLayer* audioLayer) const
{
    *audioLayer = AudioLayer::kDummyAudio;
    return 0;
}


int32_t ADMWrapper::RegisterAudioCallback(AudioTransport* audioCallback)
{
    webrtc::MutexLock lock(&mutex_);

    audio_transport_ = audioCallback;
    return 0;
}
