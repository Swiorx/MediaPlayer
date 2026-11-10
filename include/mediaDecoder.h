//
// Created by swior on 10.11.2026.
//

#ifndef OOP_MEDIADECODER_H
#define OOP_MEDIADECODER_H

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>

struct mp4MemoryBuffer
{
    std::ifstream *file = nullptr;
    int64_t size = 0;
};

int minimp4Read(int64_t offset, void *buffer, size_t size, void *token);

class MediaDecoder
{
private:
    std::ifstream mp4FileStream;
    mp4MemoryBuffer mp4Buf{};
    void *mp4Demux = nullptr;
    int mp4VideoTrack = -1;
    int mp4AudioTrack = -1;
    unsigned int totalVideoSamples = 0;
    unsigned int nextVideoSampleIdx = 0;
    unsigned int nextAudioSampleIdx = 0;

    void *pWelsDecoder = nullptr;
    bool welsReady = false;
    std::vector<uint8_t> spsPpsAnnexB;
    std::vector<uint8_t> currentFrameRGBA;
    int videoWidth = 0;
    int videoHeight = 0;
    float videoFps = 25.0f;

    void *hAAC = nullptr;
    bool aacReady = false;
    std::vector<int16_t> audioPcmQueue;
    size_t audioPcmReadOffset = 0;
    unsigned int audioSampleRate = 44100;
    unsigned int audioChannels = 2;

    void sendParameterNalu(const void *data, int bytes);
    bool decodeVideoSample(unsigned int sampleIdx, bool extractRgba);

public:
    explicit MediaDecoder(const std::string &filePath = "");
    MediaDecoder(const MediaDecoder &) = delete;
    MediaDecoder &operator=(const MediaDecoder &) = delete;
    ~MediaDecoder();

    bool open(const std::string &filePath);
    bool decodeNextVideoFrame();
    void seekToRatio(float ratio);
    bool fillAudioStreamChunk(int16_t *outFrames, int framesNeeded);

    int getVideoWidth() const;
    int getVideoHeight() const;
    float getVideoFps() const;
    bool isAacReady() const;
    unsigned int getAudioSampleRate() const;
    unsigned int getAudioChannels() const;
    unsigned int getTotalVideoSamples() const;
    unsigned int getNextVideoSampleIdx() const;
    float getPlaybackProgress() const;
    bool hasCurrentFrame() const;
    const uint8_t *getFrameData() const;
    const std::vector<uint8_t> &getCurrentFrameRGBA() const;

    friend std::ostream &operator<<(std::ostream &os, const MediaDecoder &decoder);
};

#endif //OOP_MEDIADECODER_H
