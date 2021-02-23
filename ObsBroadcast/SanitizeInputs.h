/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
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

#ifndef SANITIZE_INPUTS_H_
#define SANITIZE_INPUTS_H_


/// Selects the optimal resolution & audio bitrate for a given video bitrate.
class SanitizeInputs
{
public:
    SanitizeInputs(int videoBitrateKbps)
        : m_videoBitrateKbps(videoBitrateKbps)
        , m_audioBitrateKbps(0)
        , m_width(0)
        , m_height(0)
    {
        OptimalFrameSize(m_videoBitrateKbps, m_width, m_height);
        OpusBitrate(m_videoBitrateKbps, m_audioBitrateKbps);
    }

    ~SanitizeInputs() = default;


    int FrameWidth() { return m_width; }
    int FrameHeight() { return m_height; }

    int AudioBitrateKbps() { return m_audioBitrateKbps; }


    void SetVideoBitrateKbps(int videoBitrateKbps)
    {
        m_videoBitrateKbps = videoBitrateKbps;
    }


    void Refresh()
    {
        OptimalFrameSize(m_videoBitrateKbps, m_width, m_height);
        OpusBitrate(m_videoBitrateKbps, m_audioBitrateKbps);
    }

    void Refresh(int videoBitrateKbps)
    {
        SetVideoBitrateKbps(videoBitrateKbps);
        Refresh();
    }


    /// Set optimal frame |width| & |height| for given |videoBitrateKbps|.
    static void OptimalFrameSize(int videoBitrateKbps, int& width, int& height)
    {
        if (videoBitrateKbps < 1000)
        {
            width = 640;
            height = 360;
        }
        else if (videoBitrateKbps < 1500)
        {
            width = 960;
            height = 540;
        }
        else if (videoBitrateKbps < 4000)
        {
            width = 1280;
            height = 720;
        }
        else
        {
            width = 1920;
            height = 1080;
        }
    }


    /// Ensure |videoBitrateKbps| & |audioBitrateKbps| are ideal for frame |height|.
    static void ConstrainBitrate(int height, int &videoBitrateKbps, int &audioBitrateKbps)
    {
        if (height > 720)                   // 1080p
        {
            audioBitrateKbps = 128;
            if (videoBitrateKbps > 6000)
                videoBitrateKbps = 6000;
            if (videoBitrateKbps < 4000)
                videoBitrateKbps = 4000;
        }
        else if (height > 540)              // 720p
        {
            audioBitrateKbps = 128;
            if (videoBitrateKbps > 2970)
                videoBitrateKbps = 2970;
            if (videoBitrateKbps < 1500)
                videoBitrateKbps = 1500;
        }
        else if (height > 360)              // 540p
        {
            audioBitrateKbps = 112;
            if (videoBitrateKbps > 1470)
                videoBitrateKbps = 1470;
            if (videoBitrateKbps < 1000)
                videoBitrateKbps = 1000;
        }
        else                                // 360p
        {
            audioBitrateKbps = 96;
            if (videoBitrateKbps > 1200)
                videoBitrateKbps = 1200;
            if (videoBitrateKbps < 350)
                videoBitrateKbps = 350;
        }
    }


    /// Ensure |videoBitrateKbps| is ideal for frame |height|.
    static void ConstrainBitrate(int height, int &videoBitrateKbps)
    {
        int audioBitrateKbps;
        ConstrainBitrate(height, videoBitrateKbps, audioBitrateKbps);
    }


    /// Set appropriate |opusBitrateKbps| for given |videoBitrateKbps|.
    static void OpusBitrate(int videoBitrateKbps, int& opusBitrateKbps)
    {
        if (videoBitrateKbps < 1000)
            opusBitrateKbps = 96;
        else if (videoBitrateKbps < 1500)
            opusBitrateKbps = 112;
        else
            opusBitrateKbps = 128;
    }

private:
    int m_videoBitrateKbps;
    int m_audioBitrateKbps;
    int m_width;
    int m_height;
};

#endif  // SANITIZE_INPUTS_H_
