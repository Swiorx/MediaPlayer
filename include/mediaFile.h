//
// Created by swior on 21.10.2026.
//

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

public:
    mediaFile(const std::string &fileName, const std::string &filePath, const std::string &fileType, int* duration, int* fileSize);

    mediaFile(const mediaFile& file);
    mediaFile& operator=(const mediaFile& file);
    ~mediaFile();

    friend std::ostream &operator<<(std::ostream &os, const mediaFile &file);
};
#endif //OOP_MEDIAFILE_H
