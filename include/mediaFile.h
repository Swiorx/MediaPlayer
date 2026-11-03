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
    explicit mediaFile(const std::string &fileName = "unknown", const std::string &filePath = "", const std::string &fileType = ".mp4", int durationVal = 1, int fileSizeVal = 1);

    mediaFile(const mediaFile& file);
    mediaFile& operator=(const mediaFile& file);
    ~mediaFile();

    bool scanMedia();
    bool scan(){return scanMedia();};
    void printStreamInfo(std::ostream &os) const;
    bool validateFileIntegrity() const;
    bool writeToCatalog(const std::string &catalogPath) const;

    double calculateBitrate() const;
    std::string formatDuration() const;
    std::string formatFileSize() const;
    std::string getResolutionPreset() const;

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
