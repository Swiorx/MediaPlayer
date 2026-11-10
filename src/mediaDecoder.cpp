//
// Created by swior on 10.11.2026.
//

#include "mediaDecoder.h"
#include <minimp4.h>
extern "C" {
#include <faad2/neaacdec.h>
}
#include <wels/codec_api.h>
#include <filesystem>
#include <cstring>

int minimp4Read(int64_t offset, void *buffer, size_t size, void *token)
{
    auto *buf = static_cast<mp4MemoryBuffer*>(token);
    if (!buf || !buf->file || offset < 0 || offset >= buf->size)
    {
        return -1;
    }
    size_t toCopy = static_cast<size_t>(buf->size - offset);
    if (toCopy > size)
    {
        toCopy = size;
    }
    buf->file->clear();
    buf->file->seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    buf->file->read(static_cast<char *>(buffer), static_cast<std::streamsize>(toCopy));
    return (static_cast<size_t>(buf->file->gcount()) == size) ? 0 : -1;
}

static void convertYuv420PlanarToRGBA(const uint8_t *yPlane, int yStride,
                                      const uint8_t *uPlane, const uint8_t *vPlane, int uvStride,
                                      int width, int height, std::vector<uint8_t> &rgbaOut)
{
    rgbaOut.resize(static_cast<size_t>(width * height * 4));
    for (int y = 0; y < height; ++y)
    {
        const uint8_t *yRow = yPlane + y * yStride;
        const uint8_t *uRow = uPlane + (y / 2) * uvStride;
        const uint8_t *vRow = vPlane + (y / 2) * uvStride;
        uint8_t *dstRow = rgbaOut.data() + static_cast<size_t>(y * width * 4);
        for (int x = 0; x < width; ++x)
        {
            const int yVal = static_cast<int>(yRow[x]);
            const int uVal = static_cast<int>(uRow[x / 2]) - 128;
            const int vVal = static_cast<int>(vRow[x / 2]) - 128;

            int r = yVal + ((359 * vVal) >> 8);
            int g = yVal - ((88 * uVal + 183 * vVal) >> 8);
            int b = yVal + ((454 * uVal) >> 8);

            if (r < 0) r = 0; else if (r > 255) r = 255;
            if (g < 0) g = 0; else if (g > 255) g = 255;
            if (b < 0) b = 0; else if (b > 255) b = 255;

            dstRow[x * 4 + 0] = static_cast<uint8_t>(r);
            dstRow[x * 4 + 1] = static_cast<uint8_t>(g);
            dstRow[x * 4 + 2] = static_cast<uint8_t>(b);
            dstRow[x * 4 + 3] = 255;
        }
    }
}

MediaDecoder::MediaDecoder(const std::string &filePath)
{
    if (!filePath.empty())
    {
        open(filePath);
    }
}

MediaDecoder::~MediaDecoder()
{
    if (hAAC)
    {
        NeAACDecClose(hAAC);
    }
    auto *wels = static_cast<ISVCDecoder*>(pWelsDecoder);
    if (welsReady && wels)
    {
        wels->Uninitialize();
    }
    if (wels)
    {
        WelsDestroyDecoder(wels);
    }
    if (mp4Demux)
    {
        auto *mp4 = static_cast<MP4D_demux_t*>(mp4Demux);
        MP4D_close(mp4);
        delete mp4;
    }
    if (mp4FileStream.is_open())
    {
        mp4FileStream.close();
    }
}

void MediaDecoder::sendParameterNalu(const void *data, int bytes)
{
    auto *wels = static_cast<ISVCDecoder*>(pWelsDecoder);
    if (!welsReady || !wels || !data || bytes <= 0)
    {
        return;
    }
    std::vector<uint8_t> nalu(4 + static_cast<size_t>(bytes));
    nalu[0] = 0;
    nalu[1] = 0;
    nalu[2] = 0;
    nalu[3] = 1;
    std::memcpy(nalu.data() + 4, data, static_cast<size_t>(bytes));
    spsPpsAnnexB.insert(spsPpsAnnexB.end(), nalu.begin(), nalu.end());

    unsigned char *pData[3] = {nullptr, nullptr, nullptr};
    SBufferInfo sDstBufInfo{};
    wels->DecodeFrameNoDelay(nalu.data(), static_cast<int>(nalu.size()), pData, &sDstBufInfo);
}

bool MediaDecoder::decodeVideoSample(unsigned int sampleIdx, bool extractRgba)
{
    auto *wels = static_cast<ISVCDecoder*>(pWelsDecoder);
    auto *mp4 = static_cast<MP4D_demux_t*>(mp4Demux);
    if (!welsReady || !wels || !mp4 || mp4VideoTrack < 0)
    {
        return false;
    }
    const MP4D_track_t &track = mp4->track[mp4VideoTrack];
    if (sampleIdx >= track.sample_count)
    {
        return false;
    }
    unsigned int frameBytes = 0, timestamp = 0, duration = 0;
    const MP4D_file_offset_t offset = MP4D_frame_offset(mp4, mp4VideoTrack, sampleIdx, &frameBytes, &timestamp, &duration);
    if (frameBytes <= 4 || static_cast<int64_t>(offset + frameBytes) > mp4Buf.size)
    {
        return false;
    }
    if (duration > 0 && track.timescale > 0)
    {
        videoFps = static_cast<float>(track.timescale) / static_cast<float>(duration);
    }
    std::vector<uint8_t> sample(frameBytes);
    mp4FileStream.clear();
    mp4FileStream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    mp4FileStream.read(reinterpret_cast<char *>(sample.data()), static_cast<std::streamsize>(frameBytes));
    if (static_cast<size_t>(mp4FileStream.gcount()) != frameBytes)
    {
        return false;
    }
    size_t pos = 0;
    while (pos + 4 <= sample.size())
    {
        const uint32_t nalLen = (static_cast<uint32_t>(sample[pos]) << 24) |
                                (static_cast<uint32_t>(sample[pos + 1]) << 16) |
                                (static_cast<uint32_t>(sample[pos + 2]) << 8) |
                                static_cast<uint32_t>(sample[pos + 3]);
        sample[pos] = 0;
        sample[pos + 1] = 0;
        sample[pos + 2] = 0;
        sample[pos + 3] = 1;
        if (nalLen == 0 || pos + 4 + nalLen > sample.size())
        {
            break;
        }
        pos += 4 + nalLen;
    }
    unsigned char *pData[3] = {nullptr, nullptr, nullptr};
    SBufferInfo sDstBufInfo{};
    wels->DecodeFrameNoDelay(sample.data(), static_cast<int>(sample.size()), pData, &sDstBufInfo);
    if (!extractRgba)
    {
        return true;
    }
    if (sDstBufInfo.iBufferStatus == 1 && pData[0] && pData[1] && pData[2])
    {
        const int w = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
        const int h = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
        const int yStride = sDstBufInfo.UsrData.sSystemBuffer.iStride[0];
        const int uvStride = sDstBufInfo.UsrData.sSystemBuffer.iStride[1];
        if (w > 0 && h > 0)
        {
            videoWidth = w;
            videoHeight = h;
            convertYuv420PlanarToRGBA(pData[0], yStride, pData[1], pData[2], uvStride, w, h, currentFrameRGBA);
            return true;
        }
    }
    return false;
}

bool MediaDecoder::decodeNextVideoFrame()
{
    auto *mp4 = static_cast<MP4D_demux_t*>(mp4Demux);
    if (!welsReady || !pWelsDecoder || !mp4 || mp4VideoTrack < 0)
    {
        return false;
    }
    const MP4D_track_t &track = mp4->track[mp4VideoTrack];
    while (nextVideoSampleIdx < track.sample_count)
    {
        if (decodeVideoSample(nextVideoSampleIdx++, true))
        {
            return true;
        }
    }
    return false;
}

void MediaDecoder::seekToRatio(float ratio)
{
    if (ratio < 0.0f)
    {
        ratio = 0.0f;
    }
    if (ratio > 1.0f)
    {
        ratio = 1.0f;
    }
    auto *wels = static_cast<ISVCDecoder*>(pWelsDecoder);
    auto *mp4 = static_cast<MP4D_demux_t*>(mp4Demux);
    if (welsReady && wels && !spsPpsAnnexB.empty())
    {
        unsigned char *pData[3] = {nullptr, nullptr, nullptr};
        SBufferInfo sDstBufInfo{};
        wels->DecodeFrameNoDelay(spsPpsAnnexB.data(), static_cast<int>(spsPpsAnnexB.size()), pData, &sDstBufInfo);
    }
    nextVideoSampleIdx = 0;
    nextAudioSampleIdx = 0;
    audioPcmQueue.clear();
    audioPcmReadOffset = 0;

    if (mp4 && mp4AudioTrack >= 0)
    {
        const unsigned int aCount = mp4->track[mp4AudioTrack].sample_count;
        nextAudioSampleIdx = static_cast<unsigned int>(ratio * static_cast<float>(aCount));
        if (nextAudioSampleIdx >= aCount && aCount > 0)
        {
            nextAudioSampleIdx = aCount - 1;
        }
    }

    if (mp4 && mp4VideoTrack >= 0 && totalVideoSamples > 0)
    {
        unsigned int targetSample = static_cast<unsigned int>(ratio * static_cast<float>(totalVideoSamples));
        if (targetSample >= totalVideoSamples)
        {
            targetSample = totalVideoSamples - 1;
        }
        const MP4D_track_t &track = mp4->track[mp4VideoTrack];
        unsigned int syncSample = 0;
        for (unsigned int idx = 0; idx <= targetSample && idx < track.sample_count; ++idx)
        {
            unsigned int frameBytes = 0, timestamp = 0, duration = 0;
            const MP4D_file_offset_t offset = MP4D_frame_offset(mp4, mp4VideoTrack, idx, &frameBytes, &timestamp, &duration);
            if (frameBytes > 4 && static_cast<int64_t>(offset + frameBytes) <= mp4Buf.size)
            {
                const size_t inspectLen = (frameBytes < 128) ? frameBytes : 128;
                std::vector<uint8_t> buf(inspectLen);
                mp4FileStream.clear();
                mp4FileStream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
                mp4FileStream.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(inspectLen));
                size_t pos = 0;
                while (pos + 5 <= inspectLen)
                {
                    const uint32_t nalLen = (static_cast<uint32_t>(buf[pos]) << 24) |
                                            (static_cast<uint32_t>(buf[pos + 1]) << 16) |
                                            (static_cast<uint32_t>(buf[pos + 2]) << 8) |
                                            static_cast<uint32_t>(buf[pos + 3]);
                    if ((buf[pos + 4] & 0x1F) == 5)
                    {
                        syncSample = idx;
                        break;
                    }
                    if (nalLen == 0 || pos + 4 + nalLen > frameBytes)
                    {
                        break;
                    }
                    pos += 4 + nalLen;
                }
            }
        }
        nextVideoSampleIdx = syncSample;
        while (nextVideoSampleIdx < targetSample)
        {
            decodeVideoSample(nextVideoSampleIdx++, false);
        }
        decodeNextVideoFrame();
    }
}

bool MediaDecoder::fillAudioStreamChunk(int16_t *outFrames, int framesNeeded)
{
    auto *mp4 = static_cast<MP4D_demux_t*>(mp4Demux);
    if (!aacReady || !hAAC || !mp4 || mp4AudioTrack < 0 || audioChannels == 0)
    {
        return false;
    }
    const size_t samplesNeeded = static_cast<size_t>(framesNeeded) * audioChannels;
    const MP4D_track_t &track = mp4->track[mp4AudioTrack];

    while ((audioPcmQueue.size() - audioPcmReadOffset) < samplesNeeded && nextAudioSampleIdx < track.sample_count)
    {
        unsigned int frameBytes = 0, timestamp = 0, duration = 0;
        const MP4D_file_offset_t offset = MP4D_frame_offset(mp4, mp4AudioTrack, nextAudioSampleIdx++, &frameBytes, &timestamp, &duration);
        if (frameBytes > 0 && static_cast<int64_t>(offset + frameBytes) <= mp4Buf.size)
        {
            std::vector<uint8_t> aacPacket(frameBytes);
            mp4FileStream.clear();
            mp4FileStream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
            mp4FileStream.read(reinterpret_cast<char *>(aacPacket.data()), static_cast<std::streamsize>(frameBytes));
            if (static_cast<size_t>(mp4FileStream.gcount()) == frameBytes)
            {
                NeAACDecFrameInfo frameInfo{};
                void *pcm = NeAACDecDecode(hAAC, &frameInfo, aacPacket.data(), frameBytes);
                if (frameInfo.error == 0 && frameInfo.samples > 0 && pcm != nullptr)
                {
                    const auto *samplesPtr = static_cast<const int16_t *>(pcm);
                    audioPcmQueue.insert(audioPcmQueue.end(), samplesPtr, samplesPtr + frameInfo.samples);
                }
            }
        }
    }

    const size_t available = audioPcmQueue.size() - audioPcmReadOffset;
    if (available == 0)
    {
        return false;
    }

    const size_t toCopy = (available >= samplesNeeded) ? samplesNeeded : available;
    std::memcpy(outFrames, audioPcmQueue.data() + audioPcmReadOffset, toCopy * sizeof(int16_t));
    if (toCopy < samplesNeeded)
    {
        std::memset(outFrames + toCopy, 0, (samplesNeeded - toCopy) * sizeof(int16_t));
    }
    audioPcmReadOffset += toCopy;

    if (audioPcmReadOffset >= 32768)
    {
        audioPcmQueue.erase(audioPcmQueue.begin(), audioPcmQueue.begin() + static_cast<ptrdiff_t>(audioPcmReadOffset));
        audioPcmReadOffset = 0;
    }
    return true;
}

bool MediaDecoder::open(const std::string &filePath)
{
    if (filePath.empty() || filePath.find(".mp4") == std::string::npos || !std::filesystem::exists(std::filesystem::path(filePath)))
    {
        return false;
    }

    ISVCDecoder *wels = nullptr;
    if (WelsCreateDecoder(&wels) == 0 && wels != nullptr)
    {
        pWelsDecoder = wels;
        SDecodingParam sDecParam{};
        sDecParam.sVideoProperty.size = sizeof(sDecParam.sVideoProperty);
        sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
        sDecParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
        int traceLevel = WELS_LOG_QUIET;
        wels->SetOption(DECODER_OPTION_TRACE_LEVEL, &traceLevel);
        welsReady = (wels->Initialize(&sDecParam) == 0);
    }

    mp4FileStream.open(std::filesystem::path(filePath), std::ios::binary | std::ios::ate);
    if (!mp4FileStream.is_open())
    {
        return false;
    }
    const std::streamsize fSize = mp4FileStream.tellg();
    mp4FileStream.seekg(0, std::ios::beg);
    if (fSize <= 16)
    {
        return false;
    }

    mp4Buf = {&mp4FileStream, static_cast<int64_t>(fSize)};
    auto *mp4 = new MP4D_demux_t{};
    if (MP4D_open(mp4, minimp4Read, &mp4Buf, mp4Buf.size) == 0)
    {
        delete mp4;
        return false;
    }
    mp4Demux = mp4;

    for (unsigned int tr = 0; tr < mp4->track_count; ++tr)
    {
        const MP4D_track_t &track = mp4->track[tr];
        if (track.handler_type == MP4D_HANDLER_TYPE_VIDE && mp4VideoTrack < 0 && welsReady)
        {
            mp4VideoTrack = static_cast<int>(tr);
            totalVideoSamples = track.sample_count;

            int bytes = 0;
            int i = 0;
            while (const void *sps = MP4D_read_sps(mp4, tr, i++, &bytes))
            {
                sendParameterNalu(sps, bytes);
            }
            i = 0;
            while (const void *pps = MP4D_read_pps(mp4, tr, i++, &bytes))
            {
                sendParameterNalu(pps, bytes);
            }
        }
        else if (track.handler_type == MP4D_HANDLER_TYPE_SOUN && mp4AudioTrack < 0 && track.dsi && track.dsi_bytes > 0)
        {
            hAAC = NeAACDecOpen();
            if (hAAC)
            {
                unsigned long srate = 0;
                unsigned char chans = 0;
                if (NeAACDecInit2(hAAC, track.dsi, track.dsi_bytes, &srate, &chans) >= 0)
                {
                    audioSampleRate = static_cast<unsigned int>(srate > 0 ? srate : 44100);
                    audioChannels = static_cast<unsigned int>(chans > 0 ? chans : 2);
                    mp4AudioTrack = static_cast<int>(tr);
                    aacReady = true;
                }
            }
        }
    }

    decodeNextVideoFrame();
    return true;
}

int MediaDecoder::getVideoWidth() const
{
    return videoWidth;
}

int MediaDecoder::getVideoHeight() const
{
    return videoHeight;
}

float MediaDecoder::getVideoFps() const
{
    return videoFps;
}

bool MediaDecoder::isAacReady() const
{
    return aacReady;
}

unsigned int MediaDecoder::getAudioSampleRate() const
{
    return audioSampleRate;
}

unsigned int MediaDecoder::getAudioChannels() const
{
    return audioChannels;
}

unsigned int MediaDecoder::getTotalVideoSamples() const
{
    return totalVideoSamples;
}

unsigned int MediaDecoder::getNextVideoSampleIdx() const
{
    return nextVideoSampleIdx;
}

float MediaDecoder::getPlaybackProgress() const
{
    if (totalVideoSamples > 0)
    {
        return static_cast<float>(nextVideoSampleIdx) / static_cast<float>(totalVideoSamples);
    }
    return 0.0f;
}

bool MediaDecoder::hasCurrentFrame() const
{
    return !currentFrameRGBA.empty();
}

const uint8_t *MediaDecoder::getFrameData() const
{
    return currentFrameRGBA.data();
}

const std::vector<uint8_t> &MediaDecoder::getCurrentFrameRGBA() const
{
    return currentFrameRGBA;
}

std::ostream &operator<<(std::ostream &os, const MediaDecoder &decoder)
{
    os << "MediaDecoder [Rezolutie: " << decoder.videoWidth << "x" << decoder.videoHeight
       << ", FPS: " << decoder.videoFps
       << ", AAC: " << (decoder.aacReady ? "Activ" : "Inactiv")
       << " (" << decoder.audioSampleRate << "Hz, " << decoder.audioChannels << "ch)]\n";
    return os;
}


