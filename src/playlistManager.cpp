//
// Created by swior on 21.10.2026.
//

#include "playlistManager.h"
#include <fstream>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <ostream>





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
        std::string alt ="../" + configFile;
        file.open(alt);
        if (file.is_open())
        {
            configFile = alt;
        }
    }

    if (file.is_open())
    {
        std::string line;
        bool hasKeyValue = false;
        std::vector<std::string> whiteLines;

        while (std::getline(file,line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            auto start = line.find_first_not_of(" \t");
            if (start == std::string::npos || line[start] == '#') continue;
            line = line.substr(start);
            auto end = line.find_last_not_of(" \t");
            if (end!= std::string::npos)
            {
                line = line.substr(0, end+1);
            }

            auto eqPos = line.find('=');
            if (eqPos != std::string::npos)
            {
                hasKeyValue = true;
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos+1);

                auto kStart = key.find_first_not_of(" \t");
                auto kEnd = key.find_last_not_of(" \t");
                if (kStart != std::string::npos && kEnd != std::string::npos)
                {
                    key = key.substr(kStart, kEnd-kStart + 1);
                }

                auto vStart = value.find_first_not_of(" \t");
                auto vEnd = value.find_last_not_of(" \t");
                if (vStart != std::string::npos && vEnd != std::string::npos)
                {
                    value = value.substr(vStart, vEnd-vStart + 1);
                }

                std::transform(key.begin(),key.end(),key.begin(), [](unsigned char c){return static_cast<char>(std::toupper(c)); });
                if (key == "FOLDER")
                {
                    folderPath = value;
                }
                else if (key == "VOLUME")
                {
                    try
                    {
                        settings.setVolume(std::stoi(value));
                    }
                    catch (...){}
                }
                else if (key == "SPEED")
                {
                    try
                    {
                        settings.setPlaybackSpeed(std::stod(value));
                    }
                    catch (...){}
                }
                else if (key == "WIDTH")
                {
                    try
                    {
                        settings.setCustomResolution(std::stoi(value), settings.getResolutionHeight());
                    }
                    catch (...){}
                }
                else if (key == "HEIGHT")
                {
                    try
                    {
                        settings.setCustomResolution(settings.getResolutionWidth(), std::stoi(value));
                    }
                    catch (...){}
                }
                else if (key == "FULLSCREEN")
                {
                    try
                    {
                        settings.setFullscreen(std::stoi(value) != 0);
                    }
                    catch (...){}
                }
                else if (key == "PRESET")
                {
                    settings.applyResolutionPreset(value);
                }
            }
            else
            {
                whiteLines.push_back(line);
            }
        }
        file.close();

        if (!hasKeyValue && !whiteLines.empty())
        {
            folderPath = whiteLines[0];
            if (whiteLines.size() > 1)
            {
                std::istringstream iss(whiteLines[1]);
                int vol = 10;
                double speed = 1;
                int w = 1280;
                int h = 720;
                int fs = 0;
                std::string preset;
                if (iss >> vol >> speed >> w >> h >> fs >> preset)
                {
                    settings.setVolume(vol);
                    settings.setPlaybackSpeed(speed);
                    settings.setCustomResolution(w,h);
                    settings.setFullscreen(fs != 0);
                    settings.applyResolutionPreset(preset);
                }
            }
        }

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
    if (std::ofstream outFile(configFile, std::ios::trunc); outFile.is_open())
    {
        outFile<<newPath;
        outFile.close();
        if (!outFile.good())
        {
            std::cerr << "Eroare la scriere sau inchiderea fisierului";
        }
        else
        {
            folderPath = newPath;
            saveConfig();
            updateMediaList();
        }
    }
    else
    {
        std::cerr<<"Nu s-a putut deschide fisierul";
    }
}


void PlaylistManager::updateMediaList()
{
    mediaList.clear();
    clearQueue();
    try
    {
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

void PlaylistManager::playNextInQueue(std::ostream &os)
{
    if (mediaPlaylist.empty())
    {
        os << "Coada este goala" << "\n";
        return;
    }
    mediaFile  current = mediaPlaylist.front();
    mediaPlaylist.pop();

    current.scanMedia();

    const int realDuration = settings.calculateEffectiveDuration(current.getDuration());
    settings.displayPlayerScreen(current.getFileName() + current.getFileType(), current.getCodecName(), current.getDuration(), 0);
    os << "Element din coada: " << current << "Durata la viteza curenta: " << realDuration << "s" << "\n";
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

std::vector<mediaFile> PlaylistManager::searchLibrary(const std::string& keyword) const
{
    std::vector<mediaFile> rez;
    if (keyword.empty()) return rez;

    std::string lowerKeyword = keyword;
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), [](const unsigned char c){return std::tolower(c); });

    for (const auto &f : mediaList)
    {
        std::string lowerName = f.getFileName();
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](const unsigned char c){return std::tolower(c); });
        if (lowerName.find(lowerKeyword) != std::string::npos || f.getFileType().find(lowerKeyword) != std::string::npos)
        {
            rez.push_back(f);
        }
    }
    return rez;

}

std::vector<mediaFile> PlaylistManager::filterByCodec(const std::string& codec) const
{
    std::vector<mediaFile> rez;
    for (const auto &f : mediaList)
    {
        if (f.getCodecName() == codec || f.getCodecName().find(codec) != std::string::npos)
        {
            rez.push_back(f);
        }
    }
    return rez;
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

bool PlaylistManager::exportLibraryCatalog(const std::string& catalogPath) const
{
    std::ofstream catalog(catalogPath);
    if (!catalog.is_open()) return false;

    catalog << "Catalog Media Player" << "\n";
    catalog << "Numar fisiere: " << mediaList.size() << "\n";
    for (const auto &f : mediaList)
    {
        catalog << f.getFileName() << f.getFileType() << " " << f.getCodecName() << " " << f.getDuration() << "s" << f.getFilePath() << "\n";
    }
    return true;
}

std::string PlaylistManager::getFolderPath() const
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