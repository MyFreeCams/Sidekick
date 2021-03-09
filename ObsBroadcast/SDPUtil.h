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

#ifndef SDP_UTIL_H_
#define SDP_UTIL_H_

// clang-format off
#include <cstring>
#include <regex>
#include <sstream>
#include <string>
#include <vector>


/// SDP modification utilities: constrain bitrate, filter payloads, enable stereo.
class SDPUtil
{
public:
    static void EnableStereo(std::string& sdp)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        std::string opusRe = "a=rtpmap:(\\d+)\\s+opus";
        int rtpmap = FindLineRegEx(sdpLines, opusRe);
        if (rtpmap == -1)
            return;
        std::smatch match;
        std::regex re(opusRe, std::regex::flag_type::icase);
        if (!std::regex_search(sdpLines[rtpmap], match, re))
            return;
        std::string payloadNumber = match[1].str();
        std::vector<int> fmtp = FindLines(sdpLines, "a=fmtp:" + payloadNumber);
        // There is at least 1 fmtp line for this payload type.
        if (fmtp[0] != -1)
        {
            for (const int& i : fmtp)
            {
                sdpLines[i] = sdpLines[i].append(";stereo=1;sprop-stereo=1");
                sdpLines[i] = sdpLines[i].append(";maxplaybackrate=48000");
                sdpLines[i] = sdpLines[i].append(";sprop-maxcapturerate=48000");
            }
        }
        // No fmtp line found for this payload type.
        else
        {
            std::string fmtpLine = "a=fmtp:" + payloadNumber;
            sdpLines.insert(sdpLines.begin() + rtpmap + 1, fmtpLine);
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(" minptime=10;useinbandfec=1");
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";stereo=1;sprop-stereo=1");
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";maxplaybackrate=48000");
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";sprop-maxcapturerate=48000");
        }
        sdp = Join(sdpLines, "\r\n");
    }


    /// Enables stereo. Disables FEC.
    static void EnableStereoDisableFec(std::string& sdp)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        std::string opusRe = "a=rtpmap:(\\d+)\\s+opus";
        int rtpmap = FindLineRegEx(sdpLines, opusRe);
        if (rtpmap == -1)
            return;
        std::smatch match;
        std::regex re(opusRe, std::regex::flag_type::icase);
        if (!std::regex_search(sdpLines[rtpmap], match, re))
            return;
        std::string payloadNumber = match[1].str();
        std::vector<int> fmtp = FindLines(sdpLines, "a=fmtp:" + payloadNumber);
        // There is at least 1 fmtp line for this payload type.
        if (fmtp[0] != -1)
        {
            for (const int& i : fmtp)
            {
                if (sdpLines[i].find(std::string("useinbandfec=1")) != std::string::npos)
                {
                    std::string fmtpLine = "a=fmtp:" + payloadNumber;
                    sdpLines.insert(sdpLines.begin() + i + 1, fmtpLine);
                    sdpLines[i+1] = sdpLines[i+1].append(" minptime=10;useinbandfec=0");
                    sdpLines[i+1] = sdpLines[i+1].append(";sprop-stereo=1;stereo=1");
                    sdpLines[i+1] = sdpLines[i+1].append(";dtx=0;ptime=10;maxptime=10");
                    sdpLines[i+1] = sdpLines[i+1].append(";sprop-maxcapturerate=48000");
                    sdpLines[i+1] = sdpLines[i+1].append(";maxplaybackrate=48000");
                    sdpLines.erase(sdpLines.begin() + i);
                }
            }
        }
        // No fmtp line for this payload type.
        else
        {
            std::string fmtpLine = "a=fmtp:" + payloadNumber;
            sdpLines.insert(sdpLines.begin() + rtpmap + 1, fmtpLine);
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(" minptime=10;useinbandfec=0");
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";sprop-stereo=1;stereo=1");
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";dtx=0;ptime=10;maxptime=10");
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";sprop-maxcapturerate=48000");
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";maxplaybackrate=48000");
        }
        sdp = Join(sdpLines, "\r\n");
    }


    /// Sets audio bitrate constraints (b=AS,CT,TIAS).
    static void ConstrainAudioBitrateAS(std::string& sdp, int bitrateKbps, bool addGoogleConstraints)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        int aLine = FindLine(sdpLines, "m=audio ");
        int vLine = FindLine(sdpLines, "m=video ");
        int audioStart = aLine > 0 ? aLine : 0;
        int audioEnd = vLine > audioStart ? vLine : sdpLines.size();
        int testLineAS = FindLine(sdpLines, "b=AS:");
        int testLineCT = FindLine(sdpLines, "b=CT:");
        int testLineTIAS = FindLine(sdpLines, "b=TIAS:");
        std::ostringstream lineAS, lineCT, lineTIAS;
        lineAS << "b=AS:" << bitrateKbps;
        lineCT << "b=CT:" << bitrateKbps;
        lineTIAS << "b=TIAS:" << bitrateKbps * 1000;
        // Audio section does not have b=AS,CT,TIAS constraints.
        if ((testLineAS < audioStart || testLineAS > audioEnd) &&
            (testLineCT < audioStart || testLineCT > audioEnd) &&
            (testLineTIAS < audioStart || testLineTIAS > audioEnd))
        {
            if (strncmp(sdpLines[aLine+1].c_str(), "c=", 2) == 0)
            {
                // Insert AS, CT, TIAS constraints below c-line.
                sdpLines.insert(sdpLines.begin() + aLine + 2, lineTIAS.str());
                sdpLines.insert(sdpLines.begin() + aLine + 2, lineCT.str());
                sdpLines.insert(sdpLines.begin() + aLine + 2, lineAS.str());
            }
            else
            {
                // Insert AS, CT, TIAS constraints below m-line.
                sdpLines.insert(sdpLines.begin() + aLine + 1, lineTIAS.str());
                sdpLines.insert(sdpLines.begin() + aLine + 1, lineCT.str());
                sdpLines.insert(sdpLines.begin() + aLine + 1, lineAS.str());
            }
        }
        else
        {
            bool foundAS = false, foundCT = false, foundTIAS = false;
            // Audio section contains b=AS. Replace line with new b=AS constraint.
            if (testLineAS > audioStart && testLineAS < audioEnd)
            {
                foundAS = true;
                sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineAS.str());
                sdpLines.erase(sdpLines.begin() + testLineAS);
            }
            // Audio section contains b=CT. Replace line with new b=CT constraint.
            if (testLineCT > audioStart && testLineCT < audioEnd)
            {
                foundCT = true;
                sdpLines.insert(sdpLines.begin() + testLineCT + 1, lineCT.str());
                sdpLines.erase(sdpLines.begin() + testLineCT);
            }
            // Audio section contains b=TIAS. Replace line with new b=TIAS constraint.
            if (testLineTIAS > audioStart && testLineTIAS < audioEnd)
            {
                foundTIAS = true;
                sdpLines.insert(sdpLines.begin() + testLineTIAS + 1, lineTIAS.str());
                sdpLines.erase(sdpLines.begin() + testLineTIAS);
            }
            if (foundAS)
            {
                if (foundCT)
                {
                    sdpLines.insert(sdpLines.begin() + testLineCT + 1, lineTIAS.str());
                }
                else
                {
                    sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineTIAS.str());
                    sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineCT.str());
                }
            }
            else if (foundCT)
            {
                if (foundTIAS)
                {
                    sdpLines.insert(sdpLines.begin() + testLineCT - 1, lineAS.str());
                }
                else
                {
                    sdpLines.insert(sdpLines.begin() + testLineCT + 1, lineTIAS.str());
                    sdpLines.insert(sdpLines.begin() + testLineCT - 1, lineAS.str());
                }
            }
            else
            {
                sdpLines.insert(sdpLines.begin() + testLineTIAS - 1, lineCT.str());
                sdpLines.insert(sdpLines.begin() + testLineTIAS - 1, lineAS.str());
            }
        }
        std::string aBitrate = std::to_string(bitrateKbps);
        std::string maxAvgBitrate = std::to_string(bitrateKbps * 1000);
        std::smatch match;
        std::string opusRe = "a=rtpmap:(\\d+)\\s+opus";
        std::regex re(opusRe, std::regex::flag_type::icase);
        int rtpmap = FindLineRegEx(sdpLines, opusRe);
        if (!std::regex_search(sdpLines[rtpmap], match, re))
            return;
        std::string payloadNumber = match[1].str();
        std::vector<int> fmtp = FindLines(sdpLines, "a=fmtp:" + payloadNumber);
        // There is at least 1 fmtp line for this payload type.
        if (fmtp[0] != -1)
        {
            bool hasMaxAvgBitrate = false;
            for (const int& i : fmtp)
            {
                std::smatch matchBitrate;
                std::string regExStr = "(a=fmtp:\\d+\\s+\\S*)(?:maxaveragebitrate=\\d+)(\\S*)";
                std::regex reBitrate(regExStr, std::regex::flag_type::icase);
                // fmtp line contains maxaveragebitrate. Replace ftmp line with new constraint.
                if (std::regex_search(sdpLines[i], matchBitrate, reBitrate))
                {
                    hasMaxAvgBitrate = true;
                    std::string pre = match[1].str();
                    std::string post = match[2].str();
                    std::ostringstream newLine;
                    newLine << pre << "maxaveragebitrate=" << maxAvgBitrate;
                    if (addGoogleConstraints)
                    {
                        newLine << ";x-google-min-bitrate=" << aBitrate;
                        newLine << ";x-google-max-bitrate=" << aBitrate;
                    }
                    newLine << post;
                    sdpLines.insert(sdpLines.begin() + i + 1, newLine.str());
                    sdpLines.erase(sdpLines.begin() + i);
                }
            }
            // No fmtp line for this payload contains maxaveragebitrate. Append bitrate constraint.
            if (!hasMaxAvgBitrate)
            {
                sdpLines[fmtp[0]] = sdpLines[fmtp[0]].append(";maxaveragebitrate=" + maxAvgBitrate);
                if (addGoogleConstraints)
                {
                    sdpLines[fmtp[0]] = sdpLines[fmtp[0]].append(";x-google-min-bitrate=" + aBitrate);
                    sdpLines[fmtp[0]] = sdpLines[fmtp[0]].append(";x-google-max-bitrate=" + aBitrate);
                }
            }
        }
        // No fmtp line found for this payload type. Insert fmtp line below rtpmap line.
        else
        {
            std::string fmtpLine = "a=fmtp:" + payloadNumber;
            sdpLines.insert(sdpLines.begin() + rtpmap + 1, fmtpLine);
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";maxaveragebitrate=" + maxAvgBitrate);
            if (addGoogleConstraints)
            {
                sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";x-google-min-bitrate=" + aBitrate);
                sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";x-google-max-bitrate=" + aBitrate);
            }
        }
        sdp = Join(sdpLines, "\r\n");
    }


    /// Sets maxavgbitrate (and optionally google min/max) audio bitrate constraints.
    static void ConstrainAudioBitrate(std::string& sdp, int bitrateKbps, bool addGoogleConstraints)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        int aLine = FindLine(sdpLines, "m=audio ");
        int vLine = FindLine(sdpLines, "m=video ");
        int audioStart = aLine > 0 ? aLine : 0;
        int audioEnd = vLine > audioStart ? vLine : sdpLines.size();
        std::string aBitrate = std::to_string(bitrateKbps);
        std::string maxAvgBitrate = std::to_string(bitrateKbps * 1000);
        std::smatch match;
        std::string opusRe = "a=rtpmap:(\\d+)\\s+opus";
        std::regex re(opusRe, std::regex::flag_type::icase);
        int rtpmap = FindLineRegEx(sdpLines, opusRe);
        if (!std::regex_search(sdpLines[rtpmap], match, re))
            return;
        std::string payloadNumber = match[1].str();
        std::vector<int> fmtp = FindLines(sdpLines, "a=fmtp:" + payloadNumber);
        // There is at least 1 fmtp line for this payload type.
        if (fmtp[0] != -1)
        {
            bool hasMaxAvgBitrate = false;
            for (const int& i : fmtp)
            {
                std::smatch matchBitrate;
                std::string regExStr = "(a=fmtp:[0-9]+\\s+\\S*)(?:maxaveragebitrate=[0-9]+)(\\S*)";
                std::regex reBitrate(regExStr, std::regex::flag_type::icase);
                // fmtp line contains maxaveragebitrate. Replace ftmp line with new constraint.
                if (std::regex_search(sdpLines[i], matchBitrate, reBitrate))
                {
                    hasMaxAvgBitrate = true;
                    std::string pre = match[1].str();
                    std::string post = match[2].str();
                    std::ostringstream newLine;
                    newLine << pre << "maxaveragebitrate=" << maxAvgBitrate;
                    if (addGoogleConstraints)
                    {
                        newLine << ";x-google-min-bitrate=" << aBitrate;
                        newLine << ";x-google-max-bitrate=" << aBitrate;
                    }
                    newLine << post;
                    sdpLines.insert(sdpLines.begin() + i + 1, newLine.str());
                    sdpLines.erase(sdpLines.begin() + i);
                }
            }
            // No fmtp line for this payload contains maxaveragebitrate. Append bitrate constraint.
            if (!hasMaxAvgBitrate)
            {
                sdpLines[fmtp[0]] = sdpLines[fmtp[0]].append(";maxaveragebitrate=" + maxAvgBitrate);
                if (addGoogleConstraints)
                {
                    sdpLines[fmtp[0]] = sdpLines[fmtp[0]].append(";x-google-min-bitrate=" + aBitrate);
                    sdpLines[fmtp[0]] = sdpLines[fmtp[0]].append(";x-google-max-bitrate=" + aBitrate);
                }
            }
        }
        // No fmtp line found for this payload type. Insert fmtp line below rtpmap line.
        else
        {
            std::string fmtpLine = "a=fmtp:" + payloadNumber;
            sdpLines.insert(sdpLines.begin() + rtpmap + 1, fmtpLine);
            sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";maxaveragebitrate=" + maxAvgBitrate);
            if (addGoogleConstraints)
            {
                sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";x-google-min-bitrate=" + aBitrate);
                sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";x-google-max-bitrate=" + aBitrate);
            }
        }
        sdp = Join(sdpLines, "\r\n");
    }


    /// Sets audio bitrate constraints (b=AS,CT,TIAS).
    static void ConstrainAudioBitrateAS(std::string& sdp, int bitrateKbps)
    {
        ConstrainAudioBitrateAS(sdp, bitrateKbps, false);
    }


    /// Sets maxavgbitrate audio bitrate constraint.
    static void ConstrainAudioBitrate(std::string& sdp, int bitrateKbps)
    {
        ConstrainAudioBitrate(sdp, bitrateKbps, false);
    }


    /// Sets video bitrate constraint (b=AS,CT,TIAS). Also adds a=framerate if |framerate| > 0.
    static void ConstrainVideoBitrate(std::string& sdp, int bitrateKbps, int framerate)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        int aLine = FindLine(sdpLines, "m=audio ");
        int vLine = FindLine(sdpLines, "m=video ");
        int videoStart = vLine > 0 ? vLine : 0;
        int videoEnd = aLine > videoStart ? aLine : sdpLines.size();
        int testLineAS = FindLine(sdpLines, "b=AS:");
        int testLineCT = FindLine(sdpLines, "b=CT:");
        int testLineTIAS = FindLine(sdpLines, "b=TIAS:");
        std::ostringstream lineAS, lineCT, lineTIAS, lineFramerate;
        lineAS << "b=AS:" << bitrateKbps;
        lineCT << "b=CT:" << bitrateKbps;
        lineTIAS << "b=TIAS:" << bitrateKbps * 1000;
        if (framerate > 0)
            lineFramerate << "a=framerate:" << framerate;
        // Video section does not have b=AS,CT,TIAS constraints.
        if ((testLineAS < videoStart || testLineAS > videoEnd) &&
            (testLineCT < videoStart || testLineCT > videoEnd) &&
            (testLineTIAS < videoStart || testLineTIAS > videoEnd))
        {
            if (strncmp(sdpLines[vLine+1].c_str(), "c=", 2) == 0)
            {
                // Insert AS, CT, TIAS constraints below c-line.
                if (framerate > 0)
                    sdpLines.insert(sdpLines.begin() + vLine + 2, lineFramerate.str());
                sdpLines.insert(sdpLines.begin() + vLine + 2, lineTIAS.str());
                sdpLines.insert(sdpLines.begin() + vLine + 2, lineCT.str());
                sdpLines.insert(sdpLines.begin() + vLine + 2, lineAS.str());
            }
            else
            {
                // Insert AS, CT, TIAS constraints below m-line.
                if (framerate > 0)
                    sdpLines.insert(sdpLines.begin() + vLine + 1, lineFramerate.str());
                sdpLines.insert(sdpLines.begin() + vLine + 1, lineTIAS.str());
                sdpLines.insert(sdpLines.begin() + vLine + 1, lineCT.str());
                sdpLines.insert(sdpLines.begin() + vLine + 1, lineAS.str());
            }
        }
        else
        {
            bool foundAS = false, foundCT = false, foundTIAS = false;
            // Video section contains b=AS. Replace line with new b=AS constraint.
            if (testLineAS > videoStart && testLineAS < videoEnd)
            {
                foundAS = true;
                sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineAS.str());
                sdpLines.erase(sdpLines.begin() + testLineAS);
            }
            // Video section contains b=CT. Replace line with new b=CT constraint.
            if (testLineCT > videoStart && testLineCT < videoEnd)
            {
                foundCT = true;
                sdpLines.insert(sdpLines.begin() + testLineCT + 1, lineCT.str());
                sdpLines.erase(sdpLines.begin() + testLineCT);
            }
            // Video section contains b=TIAS. Replace line with new b=TIAS constraint.
            if (testLineTIAS > videoStart && testLineTIAS < videoEnd)
            {
                foundTIAS = true;
                sdpLines.insert(sdpLines.begin() + testLineTIAS + 1, lineTIAS.str());
                sdpLines.erase(sdpLines.begin() + testLineTIAS);
            }
            if (!foundAS || !foundCT || !foundTIAS)
            {
                if (foundAS)
                {
                    if (foundCT)
                    {
                        sdpLines.insert(sdpLines.begin() + testLineCT + 1, lineTIAS.str());
                    }
                    else
                    {
                        sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineTIAS.str());
                        sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineCT.str());
                    }
                }
                else if (foundCT)
                {
                    if (foundTIAS)
                    {
                        sdpLines.insert(sdpLines.begin() + testLineCT - 1, lineAS.str());
                    }
                    else
                    {
                        sdpLines.insert(sdpLines.begin() + testLineCT + 1, lineTIAS.str());
                        sdpLines.insert(sdpLines.begin() + testLineCT - 1, lineAS.str());
                    }
                }
                else
                {
                    sdpLines.insert(sdpLines.begin() + testLineTIAS - 1, lineCT.str());
                    sdpLines.insert(sdpLines.begin() + testLineTIAS - 1, lineAS.str());
                }
            }
        }
        sdp = Join(sdpLines, "\r\n");
    }


    /// Sets video bitrate constraint (b=AS,CT,TIAS).
    static void ConstrainVideoBitrate(std::string& sdp, int bitrateKbps)
    {
        int framerate = 0;
        ConstrainVideoBitrate(sdp, bitrateKbps, framerate);
    }


    /// Sets video bitrate constraint (b=AS,CT,TIAS; x-google-min/max-bitrate).
    static void ConstrainVideoBitrateMaxMin(std::string& sdp,
                                            const int bitrateKbps,
                                            const std::vector<int>& videoPayloadNumbers)
    {
        std::string kbps = std::to_string(bitrateKbps);
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        int aLine = FindLine(sdpLines, "m=audio ");
        int vLine = FindLine(sdpLines, "m=video ");
        int videoStart = vLine > 0 ? vLine : 0;
        int videoEnd = aLine > videoStart ? aLine : sdpLines.size();
        int testLineAS = FindLine(sdpLines, "b=AS:");
        int testLineCT = FindLine(sdpLines, "b=CT:");
        int testLineTIAS = FindLine(sdpLines, "b=TIAS:");
        std::ostringstream lineAS, lineCT, lineTIAS;
        lineAS << "b=AS:" << bitrateKbps;
        lineCT << "b=CT:" << bitrateKbps;
        lineTIAS << "b=TIAS:" << bitrateKbps * 1000;
        // Video section does not have b=AS,CT,TIAS constraints.
        if ((testLineAS < videoStart || testLineAS > videoEnd) &&
            (testLineCT < videoStart || testLineCT > videoEnd) &&
            (testLineTIAS < videoStart || testLineTIAS > videoEnd))
        {
            // Insert AS, CT, TIAS constraints below c-line.
            if (strncmp(sdpLines[vLine+1].c_str(), "c=", 2) == 0)
            {
                sdpLines.insert(sdpLines.begin() + vLine + 1, lineTIAS.str());
                sdpLines.insert(sdpLines.begin() + vLine + 1, lineCT.str());
                sdpLines.insert(sdpLines.begin() + vLine + 1, lineAS.str());
            }
            // Insert AS, CT, TIAS constraints below m-line.
            else
            {
                sdpLines.insert(sdpLines.begin() + vLine + 1, lineTIAS.str());
                sdpLines.insert(sdpLines.begin() + vLine + 1, lineCT.str());
                sdpLines.insert(sdpLines.begin() + vLine + 1, lineAS.str());
            }
        }
        else
        {
            bool foundAS = false, foundCT = false, foundTIAS = false;
            // Video section contains b=AS. Replace line with new b=AS constraint.
            if (testLineAS > videoStart && testLineAS < videoEnd)
            {
                foundAS = true;
                sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineAS.str());
                sdpLines.erase(sdpLines.begin() + testLineAS);
            }
            // Video section contains b=CT. Replace line with new b=CT constraint.
            if (testLineCT > videoStart && testLineCT < videoEnd)
            {
                foundCT = true;

            }
            // Video section contains b=TIAS. Replace line with new b=TIAS constraint.
            if (testLineTIAS > videoStart && testLineTIAS < videoEnd)
            {
                foundTIAS = true;
            }
            if (foundAS)
            {
                if (foundCT)
                {
                    sdpLines.insert(sdpLines.begin() + testLineCT + 1, lineCT.str());
                    sdpLines.erase(sdpLines.begin() + testLineCT);
                }
                else
                {
                    sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineTIAS.str());
                    sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineCT.str());
                }
                sdpLines.insert(sdpLines.begin() + testLineAS + 1, lineAS.str());
                sdpLines.erase(sdpLines.begin() + testLineAS);
            }
            else if (foundCT)
            {
                if (foundTIAS)
                {
                    sdpLines.insert(sdpLines.begin() + testLineTIAS + 1, lineTIAS.str());
                    sdpLines.erase(sdpLines.begin() + testLineTIAS);
                }
                else
                {
                    sdpLines.insert(sdpLines.begin() + testLineCT + 1, lineTIAS.str());
                    sdpLines.insert(sdpLines.begin() + testLineCT - 1, lineAS.str());
                }
                sdpLines.insert(sdpLines.begin() + testLineCT + 1, lineCT.str());
                sdpLines.erase(sdpLines.begin() + testLineCT);
            }
            else if (foundTIAS)
            {
                sdpLines.insert(sdpLines.begin() + testLineTIAS + 1, lineTIAS.str());
                sdpLines.erase(sdpLines.begin() + testLineTIAS);
            }
            else
            {
                sdpLines.insert(sdpLines.begin() + testLineTIAS - 1, lineCT.str());
                sdpLines.insert(sdpLines.begin() + testLineTIAS - 1, lineAS.str());
            }
        }
        int testLine = FindLineRegEx(sdpLines, "x-google-[^-]+-bitrate=");
        // Video section already has an fmtp line with x-google-*-bitrate constraint.
        // Replace |testLine| with new x-google-*-bitrate constraints.
        if (testLine >= videoStart && testLine <= videoEnd)
        {
            std::smatch match;
            std::string regExStr =
                "(a=fmtp:\\d+\\s\\S+)(?:x-google-[^-]+-bitrate=\\d+)(?:;\\s*x-google-[^-]+-bitrate=\\d+)*(\\S*)";
            std::regex re(regExStr, std::regex::flag_type::icase);
            if (std::regex_search(sdpLines[testLine], match, re))
            {
                std::string pre = match[1].str();
                std::string post = match[2].str();
                std::ostringstream newLine;
                newLine << pre << "x-google-min-bitrate=" << kbps;
                newLine << ";x-google-max-bitrate=" << kbps << post;
                sdpLines.insert(sdpLines.begin() + testLine + 1, newLine.str());
                sdpLines.erase(sdpLines.begin() + testLine);
            }
            sdp = Join(sdpLines, "\r\n");
            return;
        }
        // No existing x-google-*-bitrate constraints found.
        for (const auto& num : videoPayloadNumbers)
        {
            int rtpmap = FindLine(sdpLines, "a=rtpmap:" + std::to_string(num));
            int fmtp = FindLine(sdpLines, "a=fmtp:" + std::to_string(num));
            // There is at least 1 fmtp line for this payload type.
            // Append x-google-*-bitrate constraints to fmtp line.
            if (fmtp != -1)
            {
                sdpLines[fmtp] = sdpLines[fmtp].append(";x-google-min-bitrate=" + kbps);
                sdpLines[fmtp] = sdpLines[fmtp].append(";x-google-max-bitrate=" + kbps);
            }
            // There is no fmtp line for this payload type.
            // Insert fmtp line below rtpmap line.
            else if (rtpmap != -1)
            {
                std::string fmtpLine = "a=fmtp:" + std::to_string(num);
                sdpLines.insert(sdpLines.begin() + rtpmap + 1, fmtpLine);
                sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(" x-google-min-bitrate=" + kbps);
                sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";x-google-max-bitrate=" + kbps);
            }
        }
        sdp = Join(sdpLines, "\r\n");
    }


    /// Sets google min/max video bitrate constraint.
    static void ConstrainVideoBitrateMaxMinOnly(std::string& sdp,
                                                const int bitrateKbps,
                                                const std::vector<int>& videoPayloadNumbers)
    {
        std::string kbps = std::to_string(bitrateKbps);
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        int aLine = FindLine(sdpLines, "m=audio ");
        int vLine = FindLine(sdpLines, "m=video ");
        int videoStart = vLine > 0 ? vLine : 0;
        int videoEnd = aLine > videoStart ? aLine : sdpLines.size();
        int testLine = FindLineRegEx(sdpLines, "x-google-[^-]+-bitrate=");
        // Video section already has an fmtp line with x-google-*-bitrate constraint.
        // Replace |testLine| with new x-google-*-bitrate constraints.
        if (testLine >= videoStart && testLine <= videoEnd)
        {
            std::smatch match;
            std::string regExStr =
                "(a=fmtp:\\d+\\s+\\S+)(?:x-google-[^-]+-bitrate=\\d+)(?:;\\s*x-google-[^-]+-bitrate=\\d+)*(\\S*)";
            std::regex re(regExStr, std::regex::flag_type::icase);
            if (std::regex_search(sdpLines[testLine], match, re))
            {
                std::string pre = match[1].str();
                std::string post = match[2].str();
                std::ostringstream newLine;
                newLine << pre << "x-google-min-bitrate=" << kbps;
                newLine << ";x-google-max-bitrate=" << kbps << post;
                sdpLines.insert(sdpLines.begin() + testLine + 1, newLine.str());
                sdpLines.erase(sdpLines.begin() + testLine);
            }
            sdp = Join(sdpLines, "\r\n");
            return;
        }
        // No existing x-google-*-bitrate constraints found.
        for (const auto& num : videoPayloadNumbers)
        {
            int rtpmap = FindLine(sdpLines, "a=rtpmap:" + std::to_string(num));
            int fmtp = FindLine(sdpLines, "a=fmtp:" + std::to_string(num));
            // There is at least 1 fmtp line for this payload type.
            // Append x-google-*-bitrate constraints to fmtp line.
            if (fmtp != -1)
            {
                sdpLines[fmtp] = sdpLines[fmtp].append(";x-google-min-bitrate=" + kbps);
                sdpLines[fmtp] = sdpLines[fmtp].append(";x-google-max-bitrate=" + kbps);
            }
            // There is no fmtp line for this payload type.
            // Insert fmtp line below rtpmap line.
            else if (rtpmap != -1)
            {
                std::string fmtpLine = "a=fmtp:" + std::to_string(num);
                sdpLines.insert(sdpLines.begin() + rtpmap + 1, fmtpLine);
                sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(" x-google-min-bitrate=" + kbps);
                sdpLines[rtpmap+1] = sdpLines[rtpmap+1].append(";x-google-max-bitrate=" + kbps);
            }
        }
        sdp = Join(sdpLines, "\r\n");
    }


    /// Returns true if the protocol (TCP/UDP) of |candidate| is |protocol|.
    static bool IsProtocol(const std::string& candidate, const std::string& protocol)
    {
        std::smatch match;
        std::regex re("candidate:(\\d+)\\s+(\\d+)\\s+(tcp|udp)", std::regex::flag_type::icase);
        if (std::regex_search(candidate, match, re))
            if (CaseInsStringCompare(match[3].str(), protocol))
                return true;
        return false;
    }


    /// Extracts the protocol (TCP/UDP), IPv4 address, and port from |candidate|.
    static void ParseIceCandidate(const std::string& candidate, std::string& proto, std::string& ip, int& port)
    {
        std::smatch match;
        std::string reStr = "\\s(tcp|udp)\\s+\\S+\\s((?:(?:25[0-5]|2[0-4]\\d|[01]?\\d{1,2})\\.){3}(?:25[0-5]|2[0-4]\\d|[01]?\\d{1,2}))\\s+(\\d+)\\s";
        std::regex re(reStr, std::regex::flag_type::icase);
        if (std::regex_search(candidate, match, re))
        {
            proto   = match[1].str();
            ip      = match[2].str();
            port    = std::stoi(match[3].str());
        }
    }


    /// Removes all payloads from |sdp| except |video_codec| & |audio_codec|.
    static void ForcePayload(std::string& sdp,
                             std::vector<int>& audio_payload_numbers,
                             std::vector<int>& video_payload_numbers,
                             const std::string& audio_codec,
                             const std::string& video_codec,
                             const int h264_packetization_mode,
                             const std::string& h264_profile_level_id,
                             const int vp9_profile_id)
    {
        // Filter audio payloads.
        std::string audio_payloads;
        FilterPayloads(sdp, audio_payloads, audio_payload_numbers,
                       "audio", audio_codec, h264_packetization_mode,
                       h264_profile_level_id, vp9_profile_id);
        // Filter video payloads.
        std::string video_payloads;
        FilterPayloads(sdp, video_payloads, video_payload_numbers,
                       "video", video_codec, h264_packetization_mode,
                       h264_profile_level_id, vp9_profile_id);
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        // Replace audio m-line.
        int audioLine = FindLine(sdpLines, "m=audio");
        if (audioLine != -1)
        {
            std::ostringstream newLineA;
            newLineA << "m=audio 9 UDP/TLS/RTP/SAVPF" << audio_payloads;
            sdpLines.insert(sdpLines.begin() + audioLine + 1, newLineA.str());
            sdpLines.erase(sdpLines.begin() + audioLine);
        }
        // Replace video m-line.
        int videoLine = FindLine(sdpLines, "m=video");
        if (videoLine != -1)
        {
            std::ostringstream newLineV;
            newLineV << "m=video 9 UDP/TLS/RTP/SAVPF" << video_payloads;
            sdpLines.insert(sdpLines.begin() + videoLine + 1, newLineV.str());
            sdpLines.erase(sdpLines.begin() + videoLine);
        }
        sdp = Join(sdpLines, "\r\n");
    }


    /// Removes all payloads from |sdp| except 'Opus' and |video_codec|.
    static void ForcePayload(std::string& sdp,
                             std::vector<int>& audio_payload_numbers,
                             std::vector<int>& video_payload_numbers,
                             const std::string& video_codec)
    {
        return ForcePayload(sdp, audio_payload_numbers, video_payload_numbers,
                            "opus", video_codec, 0, "42e01f", 0);
    }


    /// Removes all payloads from |sdp| except 'Opus' and |video_codec|.
    static void ForcePayload(std::string& sdp,
                             std::vector<int>& audio_payload_numbers,
                             std::vector<int>& video_payload_numbers,
                             const std::string& video_codec,
                             const int vp9_profile_id)
    {
        return ForcePayload(sdp, audio_payload_numbers, video_payload_numbers,
                            "opus", video_codec, 0, "42e01f", vp9_profile_id);
    }


    /// Removes all payloads from |sdp| except 'Opus' and |video_codec|.
    static void ForcePayload(std::string& sdp,
                             std::vector<int>& audio_payload_numbers,
                             std::vector<int>& video_payload_numbers,
                             const std::string& video_codec,
                             const int h264_packetization_mode,
                             const std::string& h264_profile_level_id)
    {
        return ForcePayload(sdp, audio_payload_numbers, video_payload_numbers,
                            "opus", video_codec, h264_packetization_mode,
                            h264_profile_level_id, 0);
    }


    /// Removes all payloads from |sdp| except 'Opus' and |video_codec|.
    static void ForcePayload(std::string& sdp,
                             std::vector<int>& audio_payload_numbers,
                             std::vector<int>& video_payload_numbers,
                             const std::string& video_codec,
                             const int h264_packetization_mode,
                             const std::string& h264_profile_level_id,
                             const int vp9_profile_id)
    {
        return ForcePayload(sdp, audio_payload_numbers, video_payload_numbers,
                            "opus", video_codec, h264_packetization_mode,
                            h264_profile_level_id, vp9_profile_id);
    }


    /// Removes the rtcp-fb line containing |rtcpFbVal| from |sdpLines|.
    static void RemoveRtcpFb(std::vector<std::string>& sdpLines, const std::string& rtcpFbVal)
    {
        std::string reStr = std::string("a=rtcp-fb:(\\*|\\d+)\\s+") + rtcpFbVal;
        int line = 0;
        while (line != -1)
        {
            line = FindLineRegEx(sdpLines, reStr);
            if (line != -1)
                sdpLines.erase(sdpLines.begin() + line);
        }
    }


    /// Removes the rtcp-fb line containing |rtcpFbVal| from |sdp|.
    static void RemoveRtcpFb(std::string& sdp, const std::string& rtcpFbVal)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        RemoveRtcpFb(sdpLines, rtcpFbVal);
        sdp = Join(sdpLines, "\r\n");
    }


    /// Removes all lines containing |str| from |sdpLines|.
    static void RemoveLinesContaining(std::vector<std::string>& sdpLines, const std::string& str)
    {
        int line = 0;
        while (line != -1)
        {
            line = FindLine(sdpLines, str);
            if (line != -1)
                sdpLines.erase(sdpLines.begin() + line);
        }
    }


    /// Removes all lines containing |str| from |sdp|.
    static void RemoveLinesContaining(std::string& sdp, const std::string& str)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        RemoveLinesContaining(sdpLines, str);
        sdp = Join(sdpLines, "\r\n");
    }


    /// Removes all lines matching RegEx |re| from |sdpLines|.
    static void RemoveLinesMatching(std::vector<std::string>& sdpLines, const std::regex& re)
    {
        int line = 0;
        while (line != -1)
        {
            line = FindLine(sdpLines, re);
            if (line != -1)
                sdpLines.erase(sdpLines.begin() + line);
        }
    }


    /// Removes all lines matching the RegEx string |regExStr| from |sdpLines|.
    static void RemoveLinesMatching(std::vector<std::string>& sdpLines, const std::string& regExStr)
    {
        const std::regex re(regExStr, std::regex::flag_type::icase);
        RemoveLinesMatching(sdpLines, re);
    }


    /// Removes all lines matching RegEx |re| from |sdp|.
    static void RemoveLinesMatching(std::string& sdp, const std::regex& re)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        RemoveLinesMatching(sdpLines, re);
        sdp = Join(sdpLines, "\r\n");
    }


    /// Removes all lines matching the RegEx string |regExStr| from |sdp|.
    static void RemoveLinesMatching(std::string& sdp, const std::string& regExStr)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        RemoveLinesMatching(sdpLines, regExStr);
        sdp = Join(sdpLines, "\r\n");
    }


    /// Removes all payloads of |media_type| ("audio" or "video") from |sdp| except |media_codec|.
    static void FilterPayloads(std::string& sdp,
                               std::string& payloads,
                               std::vector<int>& payload_numbers,
                               const std::string& media_type,
                               const std::string& media_codec,
                               const int h264_packetization_mode,
                               const std::string& h264_profile_level_id,
                               const int vp9_profile_id)
    {
        std::vector<int> apt_payload_numbers;
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        int aLine = FindLine(sdpLines, "m=audio");
        int vLine = FindLine(sdpLines, "m=video");
        size_t for_start = 0;
        size_t for_end = sdpLines.size();
        if (media_type == "audio")
        {
            for_start = aLine > 0 ? aLine : 0;
            for_end = vLine > (int)for_start ? (size_t)vLine : sdpLines.size();
        }
        else if (media_type == "video")
        {
            for_start = vLine > 0 ? vLine : 0;
            for_end = aLine > (int)for_start ? (size_t)aLine : sdpLines.size();
        }
        for (size_t i = for_start; i < for_end; i++)
        {
            std::smatch match;
            std::string payloadRe = "a=rtpmap:(\\d+)\\s+([a-z0-9-]+)";
            std::regex re(payloadRe, std::regex::flag_type::icase);
            if (std::regex_search(sdpLines[i], match, re))
            {
                int payloadNumber = std::stoi(match[1].str());
                std::string payloadCodec = match[2].str();
                bool found = CaseInsStringCompare(media_codec, payloadCodec);
                bool all = media_codec.empty();
                GetMatchingPayloads(sdp, payloads, payload_numbers, apt_payload_numbers,
                                    payloadCodec, payloadNumber, all, found,
                                    h264_packetization_mode, h264_profile_level_id,
                                    vp9_profile_id);
            }
        }
    }


    /// Removes all payloads of |media_type| ("audio" or "video") from |sdp| except |media_codec|.
    static void FilterPayloads(std::string& sdp,
                               std::string& payloads,
                               std::vector<int>& payload_numbers,
                               const std::string& media_type,
                               const std::string& codec)
    {
        return FilterPayloads(sdp, payloads, payload_numbers, media_type,
                              codec, 0, "42e01f", 0);
    }


    /// Inserts |payloadNumber| into |payload_numbers| and appends the |payloads| string
    /// for specified payload. Inserts associated RTX payload number into |apt_payload_numbers|.
    /// Deletes all non matching payloads from |sdp|.
    static void GetMatchingPayloads(std::string& sdp,
                                    std::string& payloads,
                                    std::vector<int>& payload_numbers,
                                    std::vector<int>& apt_payload_numbers,
                                    const std::string& payloadCodec,
                                    int payloadNumber,
                                    bool all,
                                    bool found,
                                    int h264_packetization_mode,
                                    const std::string& h264_profile_level_id,
                                    int vp9_profile_id)
    {
        bool keep = false, aptKeep = false;
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        if (found)
        {
            std::string h264fmtpRegEx = std::to_string(payloadNumber);
            h264fmtpRegEx += "\\slevel-asymmetry-allowed=[0-1];\\s*packetization-mode=(\\d);\\s*profile-level-id=([0-9a-f]{6})";
            std::string vp9fmtpRegEx = std::to_string(payloadNumber) + "\\s+profile-id=(\\d)";
            int h264fmtpLine = FindLineRegEx(sdpLines, h264fmtpRegEx);
            int vp9fmtpLine = FindLineRegEx(sdpLines, vp9fmtpRegEx);
            // Determine if h264 payload has desired packetization mode & profile level id.
            if (CaseInsStringCompare("h264", payloadCodec) && h264fmtpLine != -1)
            {
                std::smatch match;
                std::regex re(h264fmtpRegEx, std::regex::flag_type::icase);
                if (std::regex_search(sdpLines[h264fmtpLine], match, re))
                {
                    if (CaseInsStringCompare(h264_profile_level_id, match[2].str())
                        && h264_packetization_mode == std::stoi(match[1].str()))
                    {
                        keep = true;
                    }
                }
            }
            // Determine if vp9 payload has desired profile id.
            else if (CaseInsStringCompare("vp9", payloadCodec) && vp9fmtpLine != -1)
            {
                std::smatch match;
                std::regex re(vp9fmtpRegEx, std::regex::flag_type::icase);
                if (std::regex_search(sdpLines[vp9fmtpLine], match, re))
                {
                    if (vp9_profile_id == std::stoi(match[1].str()))
                        keep = true;
                }
            }
            else
            {
                keep = true;
            }
        }
        else if (all)
        {
            keep = true;
        }
        int rtxPayloadNumber = 0;
        int aptLine = FindLine(sdpLines, "apt=" + std::to_string(payloadNumber));
        if (aptLine != -1)
        {
            std::smatch matchApt;
            std::regex reApt("a=fmtp:(\\d+)\\s+apt", std::regex::flag_type::icase);
            if (std::regex_search(sdpLines[aptLine], matchApt, reApt))
            {
                rtxPayloadNumber = std::stoi(matchApt[1].str());
                if (keep)
                    aptKeep = true;
            }
        }
        if (keep)
        {
            payloads += std::string(" ") + std::to_string(payloadNumber);
            payload_numbers.push_back(payloadNumber);
        }
        if (aptKeep && rtxPayloadNumber > 0)
        {
            if (!all)
                payloads += std::string(" ") + std::to_string(rtxPayloadNumber);
            apt_payload_numbers.push_back(rtxPayloadNumber);
        }
        if (!keep && !aptKeep)
        {
            const auto begin = payload_numbers.begin();
            const auto end = payload_numbers.end();
            const auto apt_begin = apt_payload_numbers.begin();
            const auto apt_end = apt_payload_numbers.end();
            // Delete payload from |sdp| if the |payloadNumber| is not in |payload_numbers| or |apt_payload_numbers|.
            if (std::find(apt_begin, apt_end, payloadNumber) == apt_end
                && std::find(begin, end, payloadNumber) == end)
            {
                DeletePayload(sdp, payloadNumber);
            }
        }
    }


    /// Removes payload |payloadNumber| from |sdpLines|.
    static void DeletePayload(std::vector<std::string>& sdpLines, int payloadNumber)
    {
        std::string reStr = std::string("a=(rtpmap|fmtp|rtcp-fb):") + std::to_string(payloadNumber) + "\\s";
        int line = 0;
        while (line != -1)
        {
            line = FindLineRegEx(sdpLines, reStr);
            if (line != -1)
                sdpLines.erase(sdpLines.begin() + line);
        }
    }


    /// Removes payload |payloadNumber| from |sdp|.
    static void DeletePayload(std::string& sdp, int payloadNumber)
    {
        std::vector<std::string> sdpLines;
        Split(sdp, "\r\n", sdpLines);
        DeletePayload(sdpLines, payloadNumber);
        sdp = Join(sdpLines, "\r\n");
    }


    /// Case-insensitive string comparison.
    static bool CaseInsStringCompare(const std::string& str1, const std::string& str2)
    {
        std::string str1Cpy(str1);
        std::string str2Cpy(str2);
        std::transform(str1Cpy.begin(), str1Cpy.end(), str1Cpy.begin(),
                       [](const unsigned char& c){ return std::tolower(c); });
        std::transform(str2Cpy.begin(), str2Cpy.end(), str2Cpy.begin(),
                       [](const unsigned char& c){ return std::tolower(c); });
        return str1Cpy == str2Cpy;
    }


    /// Case-insensitive string comparison.
    static bool CaseInsStringCompare(const char* str1, const char* str2)
    {
        return CaseInsStringCompare(std::string(str1), std::string(str2));
    }


    /// Returns the (first) line number containing |str| or -1 if there are no matching lines.
    static int FindLine(const std::vector<std::string>& sdpLines, const std::string& str)
    {
        for (size_t i = 0; i < sdpLines.size(); i++)
        {
            if (sdpLines[i].find(str) != std::string::npos)
                return i;
        }
        return -1;
    }


    /// Returns the (first) line number containing |str| or -1 if there are no matching lines.
    static int FindLine(const std::vector<std::string>& sdpLines, const char* str)
    {
        return FindLine(sdpLines, std::string(str));
    }


    /// Returns the (first) line number matching RegEx |re| or -1 if there are no matching lines.
    static int FindLine(const std::vector<std::string>& sdpLines, const std::regex& re)
    {
        for (size_t i = 0; i < sdpLines.size(); i++)
        {
            std::smatch match;
            if (std::regex_search(sdpLines[i], match, re))
                return i;
        }
        return -1;
    }


    /// Returns the (first) line number matching the RegEx string |regExStr| or -1 if there are no matching lines.
    static int FindLineRegEx(const std::vector<std::string>& sdpLines, const std::string& regExStr)
    {
        const std::regex re(regExStr, std::regex::flag_type::icase);
        return FindLine(sdpLines, re);
    }


    /// Returns the (first) line number matching the RegEx string |regExStr| or -1 if there are no matching lines.
    static int FindLineRegEx(const std::vector<std::string>& sdpLines, const char* regExStr)
    {
        const std::regex re(regExStr, std::regex::flag_type::icase);
        return FindLine(sdpLines, re);
    }


    /// Returns the (first) line number matching RegEx |re| or -1 if there are no matching lines.
    static int FindLineRegEx(const std::vector<std::string>& sdpLines, const std::regex& re)
    {
        return FindLine(sdpLines, re);
    }


    /// Returns the line numbers containing |str| or -1 if there are no matching lines.
    static std::vector<int> FindLines(const std::vector<std::string>& sdpLines, const std::string& str)
    {
        std::vector<int> res;
        for (size_t i = 0; i < sdpLines.size(); i++)
        {
            if (sdpLines[i].find(str) != std::string::npos)
                res.push_back(i);
        }
        if (res.empty())
            res.push_back(-1);
        return res;
    }


    /// Returns the line numbers matching RegEx |re| or -1 if there are no matching lines.
    static std::vector<int> FindLines(const std::vector<std::string>& sdpLines, const std::regex& re)
    {
        std::vector<int> res;
        for (size_t i = 0; i < sdpLines.size(); i++)
        {
            std::smatch match;
            if (std::regex_search(sdpLines[i], match, re))
                res.push_back(i);
        }
        if (res.empty())
            res.push_back(-1);
        return res;
    }


    /// Returns the line numbers matching the RegEx string |regExStr| or -1 if there are no matching lines.
    static std::vector<int> FindLinesRegEx(const std::vector<std::string>& sdpLines, const std::string& regExStr)
    {
        const std::regex re(regExStr, std::regex::flag_type::icase);
        return FindLines(sdpLines, re);
    }


    /// Returns the line numbers matching the RegEx string |regExStr| or -1 if there are no matching lines.
    static std::vector<int> FindLinesRegEx(const std::vector<std::string>& sdpLines, const char* regExStr)
    {
        const std::regex re(regExStr, std::regex::flag_type::icase);
        return FindLines(sdpLines, re);
    }


    /// Returns the line numbers matching RegEx |re| or -1 if there are no matching lines.
    static std::vector<int> FindLinesRegEx(const std::vector<std::string>& sdpLines, const std::regex& re)
    {
        return FindLines(sdpLines, re);
    }


    static std::string Join(const std::vector<std::string>& v, const std::string& delimiter)
    {
        std::ostringstream s;
        for (const auto& i : v)
        {
            if (&i != &v[0])
                s << delimiter;
            s << i;
        }
        s << delimiter;
        return s.str();
    }


    static std::string Join(const std::vector<std::string>& v, const char* delimiter)
    {
        return Join(v, std::string(delimiter));
    }


    static void Split(const std::string& s, const char* delimiter, std::vector<std::string>& result)
    {
        char* dup = strdup(s.c_str());
        char* token = strtok(dup, delimiter);
        while (token != nullptr)
        {
            result.emplace_back(std::string(token));
            token = strtok(nullptr, delimiter);
        }
        free(dup);
    }


    static void Split(const std::string& s, const std::string& delimiter, std::vector<std::string>& result)
    {
        Split(s, delimiter.c_str(), result);
    }


    static std::vector<std::string> Split(const std::string& s, const std::string& delimiter)
    {
        std::vector<std::string> res;
        Split(s, delimiter, res);
        return res;
    }


    static std::vector<std::string> Split(const std::string& s, const char* delimiter)
    {
        std::vector<std::string> res;
        Split(s, delimiter, res);
        return res;
    }
};

#endif  // SDP_UTIL_H_
