//
// Created by swior on 02.11.2026.
//
#include "playlist.h"
#include <algorithm>
#include <fstream>
#include <cctype>


Playlist::Playlist() : name("Playlist-ul meu"), repeatAll(false) {}

Playlist::Playlist(const std::string &name, bool repeatAll) : name(name), repeatAll(repeatAll) {}

int Playlist::findIndexByName(const std::string& fileName) const
{
    for (std::size_t i = 0; i < files.size(); ++i)
    {
        if (files[i].getFileName() == fileName) return static_cast<int>(i);
    }
    return -1;
}

void Playlist::addFile(const mediaFile &file)
{
    if (findIndexByName(file.getFileName()) == -1)
    {
        files.push_back(file);
    }
}

void Playlist::addFilePlayNext(const mediaFile& file)
{
    const int existingIndex = findIndexByName(file.getFileName());
    if (existingIndex != -1)
    {
        files.erase(files.begin() + existingIndex);
    }

    files.push_front(file);
}

bool Playlist::removeFile(const std::string& fileName)
{
    const int index = findIndexByName(fileName);
    if (index >= 0)
    {
        files.erase(files.begin() + index);
        return true;
    }
    return false;
}

void Playlist::playAndRotate(std::ostream &os)
{
    if (files.empty())
    {
        os << "Playlist gol \n";
        return;
    }

    const mediaFile current = files.front();
    files.pop_front();

    if (repeatAll)
    {
        files.push_back(current);
    }
}

int Playlist::getTotalDuration() const
{
    int total = 0;
    for (const auto &f : files)
    {
        total += f.getDuration();
    }
    return total;
}

void Playlist::sortByDuration(bool descending)
{
    std::sort(files.begin(), files.end(), [descending](const mediaFile &file1, const mediaFile &file2)
    {
        if (descending)
        {
            return file1.getDuration() > file2.getDuration();
        }
        return file1.getDuration() < file2.getDuration();
    });
}

Playlist Playlist::filterVideosOnly() const
{
    Playlist rez(name + " (Video)", repeatAll);
    for (const auto &f : files)
    {
        if (f.isVideo())
        {
            rez.addFile(f);
        }
    }
    return rez;
}

Playlist Playlist::filterAudioOnly() const
{
    Playlist rez(name + " (Audio)", repeatAll);
    for (const auto &f : files)
    {
        if (f.isAudio())
        {
            rez.addFile(f);
        }
    }
    return rez;
}

Playlist Playlist::searchByName(const std::string& keyword) const
{
    Playlist rez(name + " (Cautare:" + keyword + ")", repeatAll);
    if (keyword.empty())
        return rez;

    std::string lowerKeyword = keyword;
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), [](const unsigned char c){return std::tolower(c); });

    for (const auto &f : files)
    {
        std::string lowerName = f.getFileName();
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](const unsigned char c){return std::tolower(c); });
        if (lowerName.find(lowerKeyword) != std::string::npos)
        {
            rez.addFile(f);
        }
    }
    return rez;
}

int Playlist::getTotalFileSizeKB() const
{
    int total = 0;
    for (const auto &f : files)
    {
        total += f.getFileSize();
    }
    return total;
}

bool Playlist::containsFile(const std::string& fileName) const
{
    return (findIndexByName(fileName) != -1);
}

void Playlist::clear()
{
    files.clear();
}

bool Playlist::exportToM3U(const std::string& outputPath) const
{
    std::ofstream out(outputPath);
    if (!out.is_open()) return false;

    out<< "#EXTM3U\n";
    for (const auto &f : files)
    {
        out<< "#EXTINF:" << f.getDuration() << "," << f.getFileName() << "\n";
        out<< (f.getFilePath().empty() ? (f.getFileName() + f.getFileType()) : f.getFilePath()) << "\n";
    }
    return true;
}

std::size_t Playlist::getFileCount() const
{
    return files.size();
}

const std::string &Playlist::getName() const
{
    return name;
}

bool Playlist::isRepeatAll() const
{
    return repeatAll;
}

const std::deque<mediaFile> &Playlist::getFiles() const
{
    return files;
}

bool Playlist::toggleRepeatAll()
{
    repeatAll = !repeatAll;
    return repeatAll;
}

std::ostream &operator<<(std::ostream &os, const Playlist &playlist)
{
    os << "Nume Playlist: " << playlist.getName() << " Fisiere: " << playlist.files.size() << " Durata totala: " << playlist.getTotalDuration() << " Repeat: " << playlist.isRepeatAll() << "\n";
    for (std::size_t i = 0; i < playlist.files.size(); ++i)
    {
        os << "Fisier din playlist: " << playlist.files[i];
    }
    return os;
}
