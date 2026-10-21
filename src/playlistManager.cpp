//
// Created by swior on 21.10.2026.
//

#include "playlistManager.h"
#include <fstream>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <ostream>
PlaylistManager::PlaylistManager() : folderPath("./media")
{
    loadDefaultConfig();
    updateMediaList();
}

void PlaylistManager::loadDefaultConfig()
{
    std::ifstream file(configFile);
    if (file.is_open())
    {
        std::getline(file,folderPath);
        file.close();
        if (folderPath.empty()) folderPath = "./media";
    }
    else
    {
        std::ofstream outFile(configFile);
        if (outFile.is_open())
        {
            outFile<< "./media";
            if (!outFile.good())
            {
                std::cerr<<"Nu s-a putut modifica calea se trece la default ./media";
            }
            outFile.close();
            folderPath = "./media";
        }
        else
        {
            std::cerr<<"Nu s-a putut crea fisierul de configurare se trece la default ./media" << configFile << "\n";
            folderPath = "./media";
        }


    }

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
    std::queue<std::string> emptyQueue;
    std::swap(mediaPlaylist,emptyQueue);
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
                        std::string filePathString = entry.path().string();
                        mediaList.push_back(filePathString);
                    }
                }
            }
        }
        else
        {
            std::cerr<<"Directorul nu exista";
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr<<"Eroare la parcurgerea directorului" << e.what() << "\n";
    }
}

std::string PlaylistManager::getFolderPath() const
{
    return folderPath;
}

const std::vector<std::string>& PlaylistManager::getMediaList() const
{
    return mediaList;
}

std::ostream &operator<<(std::ostream &os, const PlaylistManager &playlistManager)
{
    os << "PlaylistManager::playlistPath=" << playlistManager.getFolderPath() << " Modifica configFile.config pentru a schimba " << playlistManager.mediaList.size() << " media items";
    return os;
}