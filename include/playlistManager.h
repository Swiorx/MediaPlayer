#ifndef OOP_PLAYLISTMANAGER_H
#define OOP_PLAYLISTMANAGER_H

#include <string>
#include <queue>
#include <iostream>
#include <vector>

#include "playerSettings.h"
#include "playlist.h"

class PlaylistManager
{
private:
    std::string configFile = "configFile.config";
    std::string folderPath;
    std::vector<mediaFile> mediaList;
    std::queue<mediaFile> mediaPlaylist;
    Playlist favouritePlaylist;
    PlayerSettings settings;


public:
    explicit PlaylistManager(const std::string &configFile, const PlayerSettings &settings = PlayerSettings{});

    void loadDefaultConfig();
    bool saveConfig(const std::string &path = "") const;
    void updateFolderPath(const std::string& newPath);
    void updateMediaList();

    void addMediaToLibrary(const mediaFile &file);
    void addFavouriteByName(const std::string &fileName);
    bool removeFavouriteByName(const std::string &fileName);
    void playNextInQueue(std::ostream &os = std::cout);
    int getEffectiveLibraryDuration() const;

    std::vector<mediaFile> searchLibrary(const std::string &keyword) const;
    std::vector<mediaFile> filterByCodec(const std::string &codec) const;
    int getLibraryTotalSizeKB() const;
    int getQueueCount() const;
    void clearQueue();
    void printLibrarySummary(std::ostream &os) const;
    bool exportLibraryCatalog(const std::string &catalogPath) const;

    const std::string& getFolderPath() const;
    const std::vector<mediaFile>& getMediaList() const;
    const Playlist &getFavouritePlaylist() const;
    const PlayerSettings &getSettings() const;
    void updateSettings(const PlayerSettings &newSettings);

    friend std::ostream &operator<<(std::ostream &os, const PlaylistManager &manager);


};

#endif //OOP_PLAYLISTMANAGER_H
