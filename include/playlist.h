#ifndef OOP_PLAYLIST_H
#define OOP_PLAYLIST_H

#include <deque>
#include <string>
#include <cstddef>
#include <iostream>
#include "mediaFile.h"

class Playlist
{
private:
    std::string name;
    std::deque<mediaFile> files;
    bool repeatAll;

    int findIndexByName(const std::string &fileName) const;

public:
    Playlist();
    explicit Playlist(const std::string &name, bool repeatAll = false);

    void addFile(const mediaFile &file);
    void addFilePlayNext(const mediaFile &file);
    bool removeFile(const std::string &fileName);

    void playAndRotate(std::ostream &os = std::cout);
    int getTotalDuration() const;
    void sortByDuration(bool descending = true);
    Playlist filterVideosOnly() const;
    Playlist filterAudioOnly() const;
    Playlist searchByName(const std::string &keyword) const;

    int getTotalFileSizeKB() const;
    double getAverageDuration() const;
    mediaFile getLongestFile() const;
    mediaFile getShortestFile() const;
    bool containsFile(const std::string &fileName) const;
    void clear();
    bool swapFiles(std::size_t index1, std::size_t index2);
    bool exportToM3U(const std::string &outputPath) const;
    bool exportToCatalog(const std::string &outputPath) const;

    std::size_t getFileCount() const;
    const std::string &getName() const;
    bool isRepeatAll() const;
    const std::deque<mediaFile> &getFiles() const;
    bool toggleRepeatAll();

    friend std::ostream &operator<<(std::ostream &os, const Playlist &playlist);
};
#endif //OOP_PLAYLIST_H
