#ifndef OOP_MEDIAFILE_H
#define OOP_MEDIAFILE_H

#include <iostream>
#include <string>

class mediaFile
{
private:
    std::string fileName;
    std::string filePath;
    std::string fileType;
    int* duration;
    int* fileSize;
    std::string codecName;

    bool validateDurationSize() const;

public:
    mediaFile();
    explicit mediaFile(const std::string &fileName, const std::string &filePath = "", const std::string &fileType = ".mp4", int durationVal = 1, int fileSizeVal = 1);

    mediaFile(const mediaFile& file);
    mediaFile& operator=(const mediaFile& file);
    ~mediaFile();

    bool scanMedia();
    void printStreamInfo(std::ostream &os) const;
    bool writeToCatalog(const std::string &catalogPath) const;

    double calculateBitrate() const;
    std::string formatDuration() const;
    std::string formatFileSize() const;

    bool isAudio() const;
    bool isVideo() const;

    bool operator==(const mediaFile& file) const;
    bool operator<(const mediaFile& file) const;

    const std::string &getFileName() const;
    const std::string &getFilePath() const;
    const std::string &getFileType() const;
    const std::string &getCodecName() const;

    int getDuration() const;
    int getFileSize() const;


    friend std::ostream &operator<<(std::ostream &os, const mediaFile &file);
};
#endif //OOP_MEDIAFILE_H
