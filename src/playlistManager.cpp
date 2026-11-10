//
// Created by swior on 21.10.2026.
//

#include "playlistManager.h"
#include <fstream>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <ostream>
#include <sstream>




PlaylistManager::PlaylistManager(const std::string &configFile, const PlayerSettings &settings) : configFile(configFile), folderPath("./media"), favouritePlaylist("Playlist favorit", true), settings(settings)
{
    loadDefaultConfig();
    updateMediaList();
}

void PlaylistManager::loadDefaultConfig()
{
    std::ifstream file(configFile);
    if (!file.is_open())
    {
        std::string alt = "assets/" + configFile;
        file.open(alt);
        if (file.is_open())
        {
            configFile = alt;
        }
    }

    if (file.is_open())
    {
        std::string line;
        while (std::getline(file,line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            const auto eqPos = line.find('=');
            if (eqPos == std::string::npos)
            {
                continue;
            }
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });

            try
            {
                if (key == "FOLDER")
                {
                    folderPath = value;
                }
                else if (key == "VOLUME")
                {
                    settings.setVolume(std::stoi(value));
                }
                else if (key == "SPEED")
                {
                    settings.setPlaybackSpeed(std::stod(value));
                }
                else if (key == "WIDTH")
                {
                    settings.setCustomResolution(std::stoi(value), settings.getResolutionHeight());
                }
                else if (key == "HEIGHT")
                {
                    settings.setCustomResolution(settings.getResolutionWidth(), std::stoi(value));
                }
                else if (key == "FULLSCREEN")
                {
                    settings.setFullscreen(std::stoi(value) != 0);
                }
                else if (key == "PRESET")
                {
                    settings.applyResolutionPreset(value);
                }
            }
            catch (...) {}
        }
        file.close();
        if (folderPath.empty())
        {
            folderPath ="./media";
        }
    }
    else
    {
        folderPath = "./media";
        saveConfig();
    }
}

bool PlaylistManager::saveConfig(const std::string& path) const
{
    const std::string &targetPath = path.empty() ? configFile : path;
    std::ofstream outFile(targetPath, std::ios::trunc);
    if (!outFile.is_open()) return false;
    outFile << "FOLDER=" << folderPath << "\n";
    outFile << "VOLUME=" << settings.getVolume() << "\n";
    outFile << "SPEED=" << settings.getPlaybackSpeed() << "\n";
    outFile << "WIDTH=" << settings.getResolutionWidth() << "\n";
    outFile << "HEIGHT=" << settings.getResolutionHeight() << "\n";
    outFile << "FULLSCREEN=" << (settings.isFullscreen() ? "1" : "0") << "\n";
    outFile << "PRESET=" << settings.getResolutionPreset() << "\n";
    return outFile.good();
}

void PlaylistManager::updateFolderPath(const std::string& newPath)
{
    folderPath = newPath;
    saveConfig();
    updateMediaList();
}


void PlaylistManager::updateMediaList()
{
    mediaList.clear();
    clearQueue();
    try
    {
        const std::filesystem::path parentMedia = std::filesystem::current_path().parent_path() / "media";
        if (std::filesystem::exists(parentMedia) && std::filesystem::exists(folderPath)) {
            std::error_code ec;
            std::filesystem::copy(parentMedia, folderPath, std::filesystem::copy_options::skip_existing, ec);
        }

        if (std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath))
        {
            for (const auto& entry : std::filesystem::directory_iterator(folderPath))
            {
                if (entry.is_regular_file())
                {
                    std::string extension = entry.path().extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char c){return std::tolower(c); });
                    if (extension == ".mp3" || extension == ".wav" || extension == ".mp4" || extension == ".mkv")
                    {
                        const std::string fileName = entry.path().stem().string();
                        const std::string filePathString = entry.path().string();
                        int sizeKB = static_cast<int>(std::filesystem::file_size(entry.path()) / 1024);
                        if (sizeKB <= 0)
                        {
                            sizeKB = 256;
                        }

                        const int estimatedDuration = (extension == ".mp4" || extension == ".mkv") ? (sizeKB / 16 + 30) : sizeKB / 8 + 60;

                        mediaFile newFile(fileName, filePathString, extension, estimatedDuration, sizeKB);
                        newFile.scanMedia();
                        mediaList.push_back(newFile);
                        mediaPlaylist.push(newFile);
                    }
                }
            }
        }
        else
        {
            std::cerr<<"Directorul nu exista" << "\n";
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr<<"Eroare la parcurgerea directorului" << e.what() << "\n";
    }
}

void PlaylistManager::addMediaToLibrary(const mediaFile& file)
{
    mediaList.push_back(file);
    mediaPlaylist.push(file);
}

void PlaylistManager::addFavouriteByName(const std::string& fileName)
{
    for (const auto &file : mediaList)
    {
        if (file.getFileName() == fileName)
        {
            favouritePlaylist.addFile(file);
            return;
        }
    }
}

bool PlaylistManager::removeFavouriteByName(const std::string& fileName)
{
    return favouritePlaylist.removeFile(fileName);
}

mediaFile PlaylistManager::playNextInQueue(std::ostream &os)
{
    if (mediaPlaylist.empty())
    {
        os << "Coada este goala" << "\n";
        return mediaFile();
    }
    mediaFile  current = mediaPlaylist.front();
    mediaPlaylist.pop();

    current.scanMedia();

    const int realDuration = settings.calculateEffectiveDuration(current.getDuration());
    settings.displayPlayerScreen(current.getFileName() + current.getFileType(), current.getCodecName(), current.getDuration(), 0);
    os << "Element din coada: " << current << "Durata la viteza curenta: " << realDuration << "s" << "\n";
    return current;
}

int PlaylistManager::getEffectiveLibraryDuration() const
{
    int totalSeconds = 0;
    for (const auto &file : mediaList)
    {
        totalSeconds += file.getDuration();
    }
    return settings.calculateEffectiveDuration(totalSeconds);
}

int PlaylistManager::getLibraryTotalSizeKB() const
{
    int total = 0;
    for (const auto &f : mediaList)
    {
        total += f.getFileSize();
    }
    return total;
}

int PlaylistManager::getQueueCount() const
{
    return static_cast<int>(mediaPlaylist.size());
}

void PlaylistManager::clearQueue()
{
    std::queue<mediaFile> emptyQueue;
    std::swap(mediaPlaylist,emptyQueue);
}

void PlaylistManager::printLibrarySummary(std::ostream& os) const
{
    int videoCount = 0;
    int audioCount = 0;
    for (const auto &f : mediaList)
        if (f.isVideo()) videoCount++;
    else if (f.isAudio()) audioCount++;

    os << "Cale director: " << folderPath << "\n";
    os << "Fisiere video: " << videoCount << " Fisiere Audio: " << audioCount << "\n";
    os << "Spatiu total ocupat: " << getLibraryTotalSizeKB() << "KB" << "\n";
    os << "Durata totala: " << getEffectiveLibraryDuration() << "s la viteza " << settings.getPlaybackSpeed() << "\n";
    os << "Elemente in coada: " << mediaPlaylist.size() << "\n";
    os << "Elemente favorite: " << favouritePlaylist.getFiles().size() << "\n";
}

const std::string& PlaylistManager::getFolderPath() const
{
    return folderPath;
}

const std::vector<mediaFile> &PlaylistManager::getMediaList() const
{
    return mediaList;
}

const Playlist& PlaylistManager::getFavouritePlaylist() const
{
    return favouritePlaylist;
}

const PlayerSettings &PlaylistManager::getSettings() const
{
    return settings;
}

void PlaylistManager::updateSettings(const PlayerSettings& newSettings)
{
    settings = newSettings;
    saveConfig();
}

std::ostream &operator<<(std::ostream &os, const PlaylistManager &playlistManager)
{
    os << "PlaylistManager::playlistPath=" << playlistManager.getFolderPath() << " Modifica configFile.config pentru a schimba " << playlistManager.mediaList.size() << " media items" << playlistManager.settings << playlistManager.favouritePlaylist << "\n";
    for (const auto &f : playlistManager.mediaList)
    {
        os << f;
    }
    return os;
}