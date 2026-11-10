//
// Created by swior on 02.11.2026.
//

#include "playerSettings.h"
#include "mediaDecoder.h"
#include <raylib.h>
#include <raygui.h>
#include <iostream>
#include <filesystem>
#include <cmath>
#include <vector>

void PlayerSettings::validateSettings()
{
    if (volume < 0)
    {
        volume = 0;
    }
    if (volume > 100)
    {
        volume = 100;
    }
    if (playbackSpeed < 0.25)
    {
        playbackSpeed = 0.25;
    }
    if (resolutionHeight <= 0 || resolutionWidth <= 0)
    {
        resolutionWidth = 50;
        resolutionHeight = 50;
    }
}

PlayerSettings::PlayerSettings(): volume(10), playbackSpeed(1.0), resolutionWidth(1280), resolutionHeight(720), fullscreen(false)
    {
        validateSettings();
    }

PlayerSettings::PlayerSettings(int volume, double playbackSpeed, int resolutionWidth, int resolutionHeight, bool fullscreen) : volume(volume), playbackSpeed(playbackSpeed), resolutionWidth(resolutionWidth), resolutionHeight(resolutionHeight), fullscreen(fullscreen)
    {
        validateSettings();
    }

void PlayerSettings::adjustVolume(int delta)
{
    volume += delta;
    validateSettings();
}

void PlayerSettings::setVolume(int newVolume)
{
    volume = newVolume;
    validateSettings();
}

void PlayerSettings::setPlaybackSpeed(double newSpeed)
{
    playbackSpeed = newSpeed;
    validateSettings();
}

void PlayerSettings::toggleFullscreen()
{
    fullscreen = !fullscreen;
}

void PlayerSettings::setFullscreen(bool Fullscreen)
{
    fullscreen = Fullscreen;
}

void PlayerSettings::applyResolutionPreset(const std::string &presetName)
{
    if (presetName == "FHD")
    {
        resolutionWidth = 1920;
        resolutionHeight = 1080;
    }
    else if (presetName == "HD")
    {
        resolutionWidth = 1280;
        resolutionHeight = 720;
    }
    else if (presetName == "4K")
    {
        resolutionWidth = 3840;
        resolutionHeight = 2160;
    }
    else std::cerr << "Preset necunoscut: " << presetName << "\n";
}

std::string PlayerSettings::getResolutionPreset() const
{
    if (resolutionWidth == 1920 && resolutionHeight == 1080)
    {
        return "FHD";
    }
    if (resolutionWidth == 1280 && resolutionHeight == 720)
    {
        return "HD";
    }
    if (resolutionWidth == 3840 && resolutionHeight == 2160)
    {
        return "4K";
    }
    return "Custom";
}

int PlayerSettings::calculateEffectiveDuration(int rawSeconds) const
{
    if (rawSeconds < 0)
    {
        return 0;
    }
    return static_cast<int>(static_cast<double>(rawSeconds) / playbackSpeed);
}

void PlayerSettings::setCustomResolution(int newWidth, int newHeight)
{
    if (newWidth > 50 && newHeight > 50)
    {
        resolutionWidth = newWidth;
        resolutionHeight = newHeight;
    }
    else
    {
        std::cerr << "Rezolutie invalida: " << newWidth << "x" << newHeight << "\n";
    }
}

void PlayerSettings::resetDefaults()
{
    volume = 10;
    playbackSpeed = 1;
    resolutionWidth = 1280;
    resolutionHeight = 720;
    fullscreen = false;
}

int PlayerSettings::getVolume() const
{
    return volume;
}

double PlayerSettings::getPlaybackSpeed() const
{
    return playbackSpeed;
}

int PlayerSettings::getResolutionWidth() const
{
    return resolutionWidth;
}

int PlayerSettings::getResolutionHeight() const
{
    return resolutionHeight;
}

bool PlayerSettings::isFullscreen() const
{
    return fullscreen;
}

void PlayerSettings::displayPlayerScreen(const std::string& title, const std::string& codec, int totalDuration, int elapsedSeconds, std::ostream &os) const
{
    const int effectiveDuration = calculateEffectiveDuration(totalDuration);
    const int effectiveElapsed = calculateEffectiveDuration(elapsedSeconds);
    os << "Rezolutie curenta: " << resolutionWidth << "x" << resolutionHeight << "\n";
    os << "Fullscreen: " << fullscreen << "\n";
    os << "Fisier redat: " << title << "\n";
    os << "Codec: " << codec << "\n";
    os << "Volum: " << volume << "\n";
    os << "Progres: " << effectiveElapsed << "s / " << effectiveDuration << "s" << "\n";
}

bool PlayerSettings::playRay(const std::string& title, const std::string& codec, int durationVal, bool interactive, const std::string &filePath) const
{
    displayPlayerScreen(title, codec, durationVal, durationVal / 2);

    if (!interactive)
    {
        return true;
    }

    SetTraceLogLevel(LOG_WARNING);
    const int winW = (resolutionWidth > 1280 && !fullscreen) ? 1280 : resolutionWidth;
    const int winH = (resolutionHeight > 720 && !fullscreen) ? 720 : resolutionHeight;
    const std::string winTitle = title.empty() ? "My Window" : (title + " - My Window");

    InitWindow(winW, winH, winTitle.c_str());
    if (!IsWindowReady())
    {
        return false;
    }

    SetTargetFPS(60);
    if (fullscreen && !IsWindowFullscreen())
    {
        ToggleFullscreen();
    }

    bool audioOpenedHere = false;
    if (!IsAudioDeviceReady())
    {
        InitAudioDevice();
        audioOpenedHere = true;
    }

    Music music = {};
    bool musicLoaded = false;
    AudioStream aacStream = {};
    bool aacStreamLoaded = false;
    std::vector<int16_t> aacChunkBuf;
    Texture2D videoTexture = {};
    bool videoTextureLoaded = false;
    MediaDecoder streamCtx{};

    const bool isVideoFile = (title.find(".mp4") != std::string::npos ||
                              title.find(".mkv") != std::string::npos ||
                              filePath.find(".mp4") != std::string::npos ||
                              filePath.find(".mkv") != std::string::npos ||
                              codec.find("h264") != std::string::npos ||
                              codec.find("video") != std::string::npos);

    if (!filePath.empty() && std::filesystem::exists(filePath) && std::filesystem::file_size(filePath) > 0)
    {
        if (isVideoFile)
        {
            streamCtx.open(filePath);
            if (streamCtx.hasCurrentFrame() && streamCtx.getVideoWidth() > 0 && streamCtx.getVideoHeight() > 0)
            {
                Image firstImg{};
                firstImg.data = const_cast<uint8_t*>(streamCtx.getFrameData());
                firstImg.width = streamCtx.getVideoWidth();
                firstImg.height = streamCtx.getVideoHeight();
                firstImg.mipmaps = 1;
                firstImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
                videoTexture = LoadTextureFromImage(firstImg);
                SetTextureFilter(videoTexture, TEXTURE_FILTER_BILINEAR);
                videoTextureLoaded = (videoTexture.id > 0);
            }

            if (streamCtx.isAacReady() && streamCtx.getAudioChannels() > 0)
            {
                SetAudioStreamBufferSizeDefault(4096);
                aacStream = LoadAudioStream(streamCtx.getAudioSampleRate(), 16, streamCtx.getAudioChannels());
                if (aacStream.buffer != nullptr)
                {
                    aacChunkBuf.resize(4096 * streamCtx.getAudioChannels(), 0);
                    SetAudioStreamVolume(aacStream, static_cast<float>(volume) / 100.0f);
                    PlayAudioStream(aacStream);
                    aacStreamLoaded = true;
                }
            }
        }
        else
        {
            music = LoadMusicStream(filePath.c_str());
            if (IsMusicValid(music))
            {
                PlayMusicStream(music);
                SetMusicVolume(music, static_cast<float>(volume) / 100.0f);
                musicLoaded = true;
            }
        }
    }

    float currentProgress = 0.0f;
    float currentVol = static_cast<float>(volume);
    bool isPaused = false;
    float frameTimer = 0.0f;

    while (!WindowShouldClose())
    {
        const int sw = GetScreenWidth();
        const int sh = GetScreenHeight();

        float sliderWidth = static_cast<float>(sw - 340);
        if (sliderWidth < 100.0f)
        {
            sliderWidth = 100.0f;
        }
        Rectangle progBounds = {130.0f, static_cast<float>(sh - 52), sliderWidth, 24.0f};
        const bool isMouseDraggingBar = IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), progBounds);

        bool requestedSeek = false;

        if (IsKeyPressed(KEY_SPACE))
        {
            isPaused = !isPaused;
        }
        if (IsKeyPressed(KEY_LEFT))
        {
            currentProgress = (currentProgress >= 0.05f) ? (currentProgress - 0.05f) : 0.0f;
            requestedSeek = true;
        }
        if (IsKeyPressed(KEY_RIGHT))
        {
            currentProgress = (currentProgress <= 0.95f) ? (currentProgress + 0.05f) : 1.0f;
            requestedSeek = true;
        }
        if (IsKeyPressed(KEY_UP))
        {
            currentVol = (currentVol <= 95.0f) ? (currentVol + 5.0f) : 100.0f;
        }
        if (IsKeyPressed(KEY_DOWN))
        {
            currentVol = (currentVol >= 5.0f) ? (currentVol - 5.0f) : 0.0f;
        }
        if (IsKeyPressed(KEY_F))
        {
            ToggleFullscreen();
        }

        if (aacStreamLoaded)
        {
            SetAudioStreamVolume(aacStream, currentVol / 100.0f);
            if (isPaused)
            {
                PauseAudioStream(aacStream);
            }
            else
            {
                ResumeAudioStream(aacStream);
                if (IsAudioStreamProcessed(aacStream))
                {
                    if (streamCtx.fillAudioStreamChunk(aacChunkBuf.data(), 4096))
                    {
                        UpdateAudioStream(aacStream, aacChunkBuf.data(), 4096);
                    }
                }
            }
        }

        if (musicLoaded)
        {
            UpdateMusicStream(music);
            SetMusicVolume(music, currentVol / 100.0f);
            if (isPaused)
            {
                PauseMusicStream(music);
            }
            else
            {
                ResumeMusicStream(music);
            }

            const float totalLen = GetMusicTimeLength(music);
            if (totalLen > 0.0f && !isPaused && !isMouseDraggingBar && !requestedSeek)
            {
                currentProgress = GetMusicTimePlayed(music) / totalLen;
                if (currentProgress >= 1.0f)
                {
                    currentProgress = 1.0f;
                    isPaused = true;
                }
            }
        }
        else if (videoTextureLoaded && !isPaused && !isMouseDraggingBar && !requestedSeek)
        {
            float rawDt = GetFrameTime();
            if (rawDt > 0.1f)
            {
                rawDt = 0.033f;
            }
            const float dt = rawDt * static_cast<float>(playbackSpeed);
            const float targetFps = (streamCtx.getVideoFps() > 1.0f) ? streamCtx.getVideoFps() : 25.0f;
            const float frameInterval = 1.0f / targetFps;
            frameTimer += dt;
            int decodedThisTick = 0;
            while (frameTimer >= frameInterval && decodedThisTick < 2)
            {
                frameTimer -= frameInterval;
                decodedThisTick++;
                if (streamCtx.decodeNextVideoFrame())
                {
                    UpdateTexture(videoTexture, streamCtx.getFrameData());
                }
                else
                {
                    streamCtx.seekToRatio(0.0f);
                    if (streamCtx.hasCurrentFrame())
                    {
                        UpdateTexture(videoTexture, streamCtx.getFrameData());
                    }
                    break;
                }
            }
            if (frameTimer > frameInterval * 2.0f)
            {
                frameTimer = 0.0f;
            }
            if (streamCtx.getTotalVideoSamples() > 0)
            {
                currentProgress = streamCtx.getPlaybackProgress();
                if (currentProgress > 1.0f)
                {
                    currentProgress = 1.0f;
                }
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        Rectangle canvas = {20.0f, 20.0f, static_cast<float>(sw - 40), static_cast<float>(sh - 110)};
        DrawRectangleRec(canvas, Color{18, 18, 22, 255});

        if (videoTextureLoaded)
        {
            const float videoAspect = 16.0f / 9.0f;
            float vidH = canvas.height;
            float vidW = vidH * videoAspect;
            if (vidW > canvas.width)
            {
                vidW = canvas.width;
                vidH = vidW / videoAspect;
            }
            const float vidX = canvas.x + (canvas.width - vidW) / 2.0f;
            const float vidY = canvas.y + (canvas.height - vidH) / 2.0f;

            Rectangle srcRec = {0.0f, 0.0f, static_cast<float>(videoTexture.width), static_cast<float>(videoTexture.height)};
            Rectangle dstRec = {vidX, vidY, vidW, vidH};
            DrawTexturePro(videoTexture, srcRec, dstRec, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        }

        Rectangle bottomBar = {0.0f, static_cast<float>(sh - 80), static_cast<float>(sw), 80.0f};
        DrawRectangleRec(bottomBar, Color{35, 35, 40, 255});

        if (GuiButton(Rectangle{20.0f, static_cast<float>(sh - 58), 90.0f, 36.0f}, isPaused ? "Play" : "Pause"))
        {
            isPaused = !isPaused;
        }

        const float prevProgress = currentProgress;
        GuiSliderBar(progBounds, nullptr, nullptr, &currentProgress, 0.0f, 1.0f);
        if (std::abs(currentProgress - prevProgress) > 0.001f)
        {
            requestedSeek = true;
        }

        GuiSliderBar(Rectangle{static_cast<float>(sw - 170), static_cast<float>(sh - 52), 120.0f, 24.0f}, "Vol", "%", &currentVol, 0.0f, 100.0f);

        EndDrawing();

        if (requestedSeek)
        {
            if (musicLoaded)
            {
                SeekMusicStream(music, currentProgress * GetMusicTimeLength(music));
            }
            else if (videoTextureLoaded)
            {
                streamCtx.seekToRatio(currentProgress);
                if (streamCtx.hasCurrentFrame())
                {
                    UpdateTexture(videoTexture, streamCtx.getFrameData());
                }
                frameTimer = 0.0f;
            }
        }
    }

    if (videoTextureLoaded)
    {
        UnloadTexture(videoTexture);
    }

    if (aacStreamLoaded)
    {
        StopAudioStream(aacStream);
        UnloadAudioStream(aacStream);
    }

    if (musicLoaded)
    {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }

    if (audioOpenedHere && IsAudioDeviceReady())
    {
        CloseAudioDevice();
    }

    CloseWindow();
    return true;
}

std::ostream &operator<<(std::ostream &os, const PlayerSettings &settings)
{
    os << "Setari: " << "\n";
    os << "Volum: " << settings.volume << "\n" << "Rezolutie " << settings.resolutionWidth << "x" << settings.resolutionHeight << "\n" << "Fullscreen: " << settings.fullscreen << "\n";
    return os;
}
