//
// Created by swior on 02.11.2026.
//

#include "../include/playerSettings.h"
#include "raylib.h"
#include "raygui.h"
#include <numeric>
#include <sstream>
#include <iomanip>
#include <iostream>

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

PlayerSettings::PlayerSettings(int volume, double playbackSpeed, int resolutionWidth, int resolutionHeight, bool fullscreen) : volume(volume), playbackSpeed(playbackSpeed), resolutionWidth(resolutionWidth), resolutionHeight(resolutionHeight), fullscreen(fullscreen)
{
    validateSettings();
}

bool PlayerSettings::initAudioEngine()
{
    InitAudioDevice();
    const bool ready = IsAudioDeviceReady();
    if (ready)
    {
        CloseAudioDevice();
    }
    return ready;
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

std::string PlayerSettings::getAspectRatio() const
{
    const int gcdVal = std::gcd(resolutionWidth, resolutionHeight);
    if (gcdVal == 0)
    {
        return "N/A";
    }
    const int aspectW = resolutionWidth / gcdVal;
    const int aspectH = resolutionHeight / gcdVal;
    return std::to_string(aspectW) + ":" + std::to_string(aspectH);
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

std::string PlayerSettings::getSettingsDescription() const
{
    std::ostringstream oss;
    oss<< "Rezolutie: "<< resolutionWidth << "x"<< resolutionHeight << " (" << getAspectRatio() << ")" << "\n";
    oss<< "Volum: "<< volume << "\n";
    oss<< "Playback Speed: "<< playbackSpeed << "\n";
    oss<<"Fullscreen: "<< fullscreen << "\n";
    return oss.str();
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
    const int barWidth = 30;
    int filled = (effectiveDuration > 0) ? (effectiveElapsed * barWidth / effectiveDuration) : 0;
    if (filled > barWidth) filled = barWidth;
    if (filled < 0) filled = 0;

    std::string progressBar;
    for (int i = 0; i < filled; i++) progressBar += "=";
    if (filled < barWidth) progressBar += ">";
    while (static_cast<int>(progressBar.size()) < barWidth) progressBar += " ";

    os << "Rezolutie curenta: " << resolutionWidth << "x" << resolutionHeight << " (" << getAspectRatio() << ")" << "\n";
    os << "Fullscreen: " << fullscreen << "\n";
    os << "Fisier redat: " << title << "\n";
    os << "Codec: " << codec << "\n";
    os << "[" << progressBar << "] " << effectiveElapsed << "s / " << effectiveDuration << "s\n";
    os << "Volum: " << volume << "\n";
}

bool PlayerSettings::playRay(const std::string& title, const std::string& codec, int durationVal, bool interactive) const
{
    displayPlayerScreen(title, codec, durationVal, durationVal / 2);

    if (!interactive)
    {
        return true;
    }

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(resolutionWidth, resolutionHeight, title.c_str());
    if (!IsWindowReady())
    {
        return false;
    }

    SetTargetFPS(60);
    if (fullscreen && !IsWindowFullscreen())
    {
        ToggleFullscreen();
    }

    float currentProgress = 0.0f;
    float currentVol = static_cast<float>(volume);
    bool isPaused = false;
    int frames = 0;

    while (!WindowShouldClose())
    {
        if (!isPaused)
        {
            currentProgress += (1.0f / (60.0f * (durationVal > 0 ? durationVal : 1.0f))) * static_cast<float>(playbackSpeed);
            if (currentProgress >= 1.0f) currentProgress = 1.0f;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangle(0,0,GetScreenWidth(),60, WHITE);
        DrawText(title.c_str(), 20, 20,20,BLACK);
        DrawText((std::string("Codec: ") + codec).c_str(), GetScreenWidth()-20, GetScreenHeight()-20,20,BLACK);

        Rectangle canvas = {40.0f,80.0f, static_cast<float>(GetScreenWidth() - 80), static_cast<float>(GetScreenHeight() - 180)};
        DrawRectangleRec(canvas, BLACK);
        DrawRectangleLinesEx(canvas,2,DARKGRAY);

        Rectangle bottomBar = {0.0f, static_cast<float>(GetScreenWidth() - 80), static_cast<float>(GetScreenHeight() - 180), 80.0f};
        DrawRectangleRec(bottomBar, YELLOW);

        if (GuiButton(Rectangle{40, static_cast<float>(GetScreenHeight() -60), 80,36}, isPaused ? "Play" : "Pause"))
        {
            isPaused = !isPaused;
        }

        GuiSlider(Rectangle{140,static_cast<float>(GetScreenWidth() - 80), static_cast<float>(GetScreenHeight() - 180), 24}, "0:00", (std::to_string(durationVal) + "s").c_str(), &currentProgress, 0.0f,1.0f);

        GuiSlider(Rectangle{static_cast<float>(GetScreenWidth() - 220), static_cast<float>(GetScreenWidth() - 55), 120, 24}, "Vol", "%", &currentVol, 0.0f,100.0f);

        EndDrawing();

        if (++frames > 3600) break;
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
