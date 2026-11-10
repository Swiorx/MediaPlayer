#ifndef OOP_PLAYERSETTINGS_H
#define OOP_PLAYERSETTINGS_H
#include <string>
#include <iostream>
class PlayerSettings
{
private:
    int volume;
    double playbackSpeed;
    int resolutionWidth;
    int resolutionHeight;
    bool fullscreen;

    void validateSettings();

public:
    PlayerSettings();
    explicit PlayerSettings(int volume, double playbackSpeed = 1.0, int resolutionWidth = 1280, int resolutionHeight = 720, bool fullscreen = false);

    void adjustVolume(int delta);
    void setVolume(int newVolume);
    void setPlaybackSpeed(double newSpeed);
    void toggleFullscreen();
    void setFullscreen(bool Fullscreen);
    void applyResolutionPreset(const std::string &presetName);
    int calculateEffectiveDuration(int rawSeconds) const;

    void setCustomResolution(int newWidth, int newHeight);
    void resetDefaults();
    void displayPlayerScreen(const std::string &title, const std::string &codec, int totalDuration, int elapsedSeconds = 0, std::ostream &os = std::cout) const;
    bool playRay(const std::string &title, const std::string &codec, int durationVal, bool interactive = false, const std::string &filePath = "") const;

    std::string getResolutionPreset() const;
    int getVolume() const;
    double getPlaybackSpeed() const;
    int getResolutionWidth() const;
    int getResolutionHeight() const;
    bool isFullscreen() const;

    friend std::ostream &operator<<(std::ostream &os, const PlayerSettings &settings);
};
#endif //OOP_PLAYERSETTINGS_H
