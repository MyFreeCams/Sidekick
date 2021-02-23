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
#pragma once

#ifndef WOWZA_STREAM_H_
#define WOWZA_STREAM_H_

#include <obs.h>

#include <cstdint>

extern "C" const char* wowza_stream_getname(void* unused);
extern "C" void* wowza_stream_create(obs_data_t* settings, obs_output_t* output);
extern "C" bool wowza_stream_start(void* data);
extern "C" void wowza_stream_stop(void* data, uint64_t ts);
extern "C" void wowza_stream_destroy(void* data);
extern "C" void wowza_receive_video(void* data, video_data* frame);
extern "C" void wowza_receive_audio(void* data, audio_data* frame);
extern "C" void wowza_receive_multitrack_audio(void* data, size_t idx, audio_data* frame);
extern "C" void wowza_stream_defaults(obs_data_t* defaults);
extern "C" obs_properties_t* wowza_stream_properties(void* unused);
extern "C" uint64_t wowza_stream_total_bytes_sent(void* data);
extern "C" int wowza_stream_dropped_frames(void* data);
extern "C" float wowza_stream_congestion(void* data);

#endif  // WOWZA_STREAM_H_
