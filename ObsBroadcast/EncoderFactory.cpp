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

#include "EncoderFactory.h"
#include "X264Encoder.h"

#include "media/base/h264_profile_level_id.h"
#include "media/base/media_constants.h"

using std::make_unique;
using std::map;
using std::string;
using std::unique_ptr;
using std::vector;
using namespace webrtc;


// static
unique_ptr<VideoEncoderFactory> EncoderFactory::Create()
{
    return make_unique<EncoderFactory>();
}


unique_ptr<VideoEncoder> EncoderFactory::CreateVideoEncoder(const SdpVideoFormat& format)
{
    return X264Encoder::Create(format);
}


vector<SdpVideoFormat> EncoderFactory::GetSupportedFormats() const
{
    const string name = cricket::kH264CodecName;
    const SdpVideoFormat::Parameters parameters = map<string, string>{ {"profile-level-id", "42e01f"} };

    return {SdpVideoFormat(name, parameters)};
}


VideoEncoderFactory::CodecInfo EncoderFactory::QueryVideoEncoder(const SdpVideoFormat& format) const
{
    return VideoEncoderFactory::CodecInfo();
}


unique_ptr<VideoEncoderFactory> CreateX264EncoderFactory()
{
    return make_unique<EncoderFactory>();
}
