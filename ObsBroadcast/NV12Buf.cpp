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

#include "NV12Buf.h"

#include "absl/types/optional.h"
#include "api/video/i420_buffer.h"
#include "rtc_base/checks.h"
#include "rtc_base/ref_counted_object.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/scale.h"

using absl::optional;
using rtc::RefCountedObject;
using rtc::scoped_refptr;
using namespace webrtc;

static const int kBufferAlignment = 64;

int NV12DataSize(int height, int stride_y, int stride_uv)
{
    return stride_y * height + stride_uv * ((height + 1) / 2);
}


NV12Buf::NV12Buf(const uint8_t* data_y, int stride_y, const uint8_t* data_uv, int stride_uv,
                 int width, int height)
    : data_y_(data_y)
    , data_uv_(data_uv)
    , stride_y_(stride_y)
    , stride_uv_(stride_uv)
    , width_(width)
    , height_(height)
{
    RTC_CHECK(data_y != nullptr);
    RTC_CHECK(data_uv != nullptr);
    RTC_DCHECK_GT(width, 0);
    RTC_DCHECK_GT(height, 0);
    RTC_DCHECK_GE(stride_y, width);
    RTC_DCHECK_GE(stride_uv, (width + width % 2));
}

NV12Buf::NV12Buf(const uint8_t* data, int stride_y, int stride_uv, int width, int height)
    : NV12Buf(data, stride_y, data + stride_y * height, stride_uv, width, height)
{}

NV12Buf::NV12Buf(const uint8_t* data, int width, int height)
    : NV12Buf(data, width, width + width % 2, width, height)
{}


scoped_refptr<NV12Buf> NV12Buf::Create(const uint8_t* data_y, int stride_y, const uint8_t* data_uv,
                                       int stride_uv, int width, int height)
{
    return new RefCountedObject<NV12Buf>(data_y, stride_y, data_uv, stride_uv, width, height);
}

scoped_refptr<NV12Buf> NV12Buf::Create(const uint8_t* data, int stride_y, int stride_uv, int width, int height)
{
    return new RefCountedObject<NV12Buf>(data, stride_y, stride_uv, width, height);
}

scoped_refptr<NV12Buf> NV12Buf::Create(const uint8_t* data, int width, int height)
{
    return new RefCountedObject<NV12Buf>(data, width, height);
}


VideoFrameBuffer::Type NV12Buf::type() const { return Type::kNV12; }

int NV12Buf::width() const { return width_; }
int NV12Buf::height() const { return height_; }

int NV12Buf::StrideY() const { return stride_y_; }
int NV12Buf::StrideUV() const { return stride_uv_; }

const uint8_t* NV12Buf::DataY() const { return data_y_; }
const uint8_t* NV12Buf::DataUV() const { return data_uv_; }

uint8_t* NV12Buf::MutableDataY() { return const_cast<uint8_t*>(DataY()); }
uint8_t* NV12Buf::MutableDataUV() { return const_cast<uint8_t*>(DataUV()); }

const I420BufferInterface* NV12Buf::GetI420() const { return nullptr; }

scoped_refptr<I420BufferInterface> NV12Buf::ToI420()
{
    auto i420_buffer = I420Buffer::Create(width(), height());
    libyuv::NV12ToI420(DataY(), StrideY(), DataUV(), StrideUV(),
                       i420_buffer->MutableDataY(), i420_buffer->StrideY(),
                       i420_buffer->MutableDataU(), i420_buffer->StrideU(),
                       i420_buffer->MutableDataV(), i420_buffer->StrideV(),
                       width(), height());
    return i420_buffer;
}


void NV12Buf::SetBlack(NV12Buf* buffer)
{
    memset(buffer->MutableDataY(), 0, NV12DataSize(buffer->height(), buffer->StrideY(), buffer->StrideUV()));
}

void NV12Buf::InitializeData()
{
    memset(MutableDataY(), 0, NV12DataSize(height_, stride_y_, stride_uv_));
}


size_t NV12Buf::UVOffset() const
{
    return static_cast<size_t>(stride_y_ * height_);
}
