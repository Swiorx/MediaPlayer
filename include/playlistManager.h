//
// Created by swior on 21.10.2026.
//

#ifndef OOP_PLAYLISTMANAGER_H
#define OOP_PLAYLISTMANAGER_H

#include <string>
#include <queue>
#include <iostream>
#include <vector>

class PlaylistManager
{
private:
    std::string configFile = "configFile.config";
    std::string folderPath;
    std::vector<std::string> mediaList;
    std::queue<std::string> mediaPlaylist;


public:
    PlaylistManager();

    void loadDefaultConfig();
    void updateFolderPath(const std::string& newPath);
    void updateMediaList();

    std::string getFolderPath() const;
    const std::vector<std::string>& getMediaList() const;

    friend std::ostream &operator<<(std::ostream &os, const PlaylistManager &manager);


};

#endif //OOP_PLAYLISTMANAGER_H
