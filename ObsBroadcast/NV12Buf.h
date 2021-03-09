/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
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

#ifndef NV12_BUF_H_
#define NV12_BUF_H_

#include <cstdint>

#include "absl/types/optional.h"
#include "api/scoped_refptr.h"
#include "api/video/video_frame_buffer.h"
#include "rtc_base/ref_counted_object.h"

using absl::optional;
using rtc::scoped_refptr;
using webrtc::I420BufferInterface;
using webrtc::NV12BufferInterface;


class NV12Buf : public NV12BufferInterface
{
public:
    static scoped_refptr<NV12Buf> Create(const uint8_t* data_y, int stride_y,
                                         const uint8_t* data_uv, int stride_uv,
                                         int width, int height);
    static scoped_refptr<NV12Buf> Create(const uint8_t* data, int stride_y,
                                         int stride_uv, int width, int height);
    static scoped_refptr<NV12Buf> Create(const uint8_t* data, int width, int height);

    Type type() const override;

    int width() const override;
    int height() const override;

    int StrideY() const override;
    int StrideUV() const override;

    const uint8_t* DataY() const override;
    const uint8_t* DataUV() const override;

    uint8_t* MutableDataY();
    uint8_t* MutableDataUV();

    scoped_refptr<I420BufferInterface> ToI420() override;
    const I420BufferInterface* GetI420() const override;

    static void SetBlack(NV12Buf* buffer);
    void InitializeData();

protected:
    NV12Buf(const uint8_t* data_y, int stride_y, const uint8_t* data_uv, int stride_uv,
            int width, int height);
    NV12Buf(const uint8_t* data, int stride_y, int stride_uv, int width, int height);
    NV12Buf(const uint8_t* data, int width, int height);
    ~NV12Buf() override = default;

private:
    size_t UVOffset() const;

    const uint8_t* data_y_;
    const uint8_t* data_uv_;
    const int stride_y_;
    const int stride_uv_;
    const int width_;
    const int height_;
};

#endif  // NV12_BUF_H_
