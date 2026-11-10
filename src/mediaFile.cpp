#include "mediaFile.h"
#include "mediaDecoder.h"
#include <minimp4.h>
#include <mkvparser/mkvparser.h>
#include <mkvparser/mkvreader.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <filesystem>

bool mediaFile::validateDurationSize() const
{
    return (*duration > 0 && *fileSize > 0);
}

mediaFile::mediaFile() : mediaFile("unknown", "", ".mp4", 1, 1){}
mediaFile::mediaFile(const std::string &fileName, const std::string &filePath, const std::string &fileType, int durationVal, int fileSizeVal) : fileName(fileName), filePath(filePath), fileType(fileType), codecName("unscanned")
{
    duration = new int(durationVal);
    fileSize = new int (fileSizeVal);

    if (!validateDurationSize())
    {
        *duration = 1;
        *fileSize = 1;
    }
}

mediaFile::mediaFile(const mediaFile &file) : fileName(file.fileName), filePath(file.filePath), fileType(file.fileType), codecName(file.codecName)
{
    duration = new int(*file.duration);
    fileSize = new int(*file.fileSize);
}

mediaFile &mediaFile::operator=(const mediaFile &file)
{
    if (this == &file)
    {
        return *this;
    }

    fileName = file.fileName;
    filePath = file.filePath;
    fileType = file.fileType;
    codecName = file.codecName;

    *duration = *file.duration;
    *fileSize = *file.fileSize;

    return *this;
}

mediaFile::~mediaFile()
{
    delete duration;
    delete fileSize;
}

bool mediaFile::scanMedia()
{
    const std::string path = filePath.empty() ? (fileName + fileType) : filePath;

    if (fileType == ".mp3" || fileType == ".wav")
    {
        codecName = (fileType == ".mp3") ? "mp3" : "pcm_s16le";
        return true;
    }

    if (fileType == ".mp4")
    {
        codecName = "h264";
        std::ifstream inFile(std::filesystem::path(path), std::ios::binary | std::ios::ate);
        if (inFile.is_open())
        {
            const std::streamsize fSize = inFile.tellg();
            inFile.seekg(0, std::ios::beg);
            if (fSize > 16)
            {
                mp4MemoryBuffer mp4Buf{&inFile, static_cast<int64_t>(fSize)};
                MP4D_demux_t mp4{};
                if (MP4D_open(&mp4, minimp4Read, &mp4Buf, mp4Buf.size))
                {
                    bool hasVideo = false;
                    bool hasAAC = false;
                    for (unsigned int i = 0; i < mp4.track_count; i++)
                    {
                        if (mp4.track[i].handler_type == MP4D_HANDLER_TYPE_VIDE)
                        {
                            hasVideo = true;
                        }
                        else if (mp4.track[i].handler_type == MP4D_HANDLER_TYPE_SOUN)
                        {
                            hasAAC = true;
                        }
                    }
                    codecName = (hasVideo && hasAAC) ? "h264+aac" : (hasAAC ? "aac" : "h264");
                    MP4D_close(&mp4);
                }
            }
        }
        return true;
    }
    if (fileType == ".mkv")
    {
        codecName = "h264/matroska";
        mkvparser::MkvReader reader;
        if (reader.Open(path.c_str()) == 0)
        {
            long long pos = 0;
            mkvparser::Segment *pSegment = nullptr;
            if (mkvparser::Segment::CreateInstance(&reader, pos, pSegment) == 0 && pSegment != nullptr)
            {
                if (pSegment->Load() >= 0)
                {
                    const mkvparser::Tracks *pTracks = pSegment->GetTracks();
                    if (pTracks != nullptr)
                    {
                        for (unsigned int i = 0; i < pTracks->GetTracksCount(); i++)
                        {
                            const mkvparser::Track *pTrack = pTracks->GetTrackByIndex(i);
                            if (pTrack != nullptr && pTrack->GetType() == mkvparser::Track::kVideo && pTrack->GetCodecId() != nullptr)
                            {
                                codecName = pTrack->GetCodecId();
                                break;
                            }
                        }
                    }
                }
                delete pSegment;
            }
            reader.Close();
        }
        return true;
    }

    codecName = "nespecificat";
    return false;
}

void mediaFile::printStreamInfo(std::ostream &os) const
{
    os << "Detalii stream pentru: " << fileName << fileType << "\n";
    os << "Cale: " << (filePath.empty() ? "nespecificata" : filePath) << "\n";
    os << "Codec detectat: " << codecName << "\n";
    os << "Durata: " << formatDuration() << " (" << *duration << "s), Marime: " << formatFileSize() << "\n";
    os << "Bitrate calculat: " << std::fixed << std::setprecision(2) << calculateBitrate() << " kbps\n";
}

bool mediaFile::writeToCatalog(const std::string& catalogPath) const
{
    std::ofstream out(catalogPath, std::ios::app);
    if (!out.is_open()) return false;
    out << fileName << " " << (filePath.empty() ? "./media/" + fileName + fileType : filePath) << " " << fileType << " " << getDuration() << " " << getFileSize() << "\n";
    return out.good();
}

double mediaFile::calculateBitrate() const
{
    if (!duration || !fileSize || *duration <= 0)
    {
        return 0.0;
    }
    return (static_cast<double>(*fileSize) * 8.0) / static_cast<double>(*duration);
}

std::string mediaFile::formatDuration() const
{
    const int m = *duration / 60;
    const int s = *duration % 60;

    const std::string minStr = (m<10) ? ("0" + std::to_string(m)) : std::to_string(m);
    const std::string secStr = (s<10) ? ("0" + std::to_string(s)) : std::to_string(s);

    return minStr + ":" + secStr;
}

std::string mediaFile::formatFileSize() const
{
    std::ostringstream oss;
    if (*fileSize >= 1024)
    {
        const double sizeMb = static_cast<double>(*fileSize) / 1024.0;
        oss << std::fixed << std::setprecision(2) << sizeMb << " MB";
    }
    else
    {
        oss << *fileSize << " KB";
    }
    return oss.str();
}

bool mediaFile::isVideo() const
{
    return (fileType == ".mp4" || fileType == ".mkv");
}

bool mediaFile::isAudio() const
{
    return (fileType == ".mp3" || fileType == ".wav");
}

bool mediaFile::operator==(const mediaFile &other) const
{
    return (fileName==other.fileName && fileType==other.fileType);
}

bool mediaFile::operator<(const mediaFile &other) const
{
    return (*duration < *other.duration);
}

const std::string &mediaFile::getFileName() const
{
    return fileName;
}
const std::string &mediaFile::getFilePath() const
{
    return filePath;
}

const std::string &mediaFile::getFileType() const
{
    return fileType;
}

int mediaFile::getDuration() const
{
    return *duration;
}

int mediaFile::getFileSize() const
{
    return *fileSize;
}

const std::string &mediaFile::getCodecName() const
{
    return codecName;
}

std::ostream &operator<<(std::ostream &os, const mediaFile &file)
{
    os << file.fileName << file.fileType << " " << file.filePath << " Durata: " << file.formatDuration() << " Bitrate: " << file.calculateBitrate() << " Codec: " << file.codecName << "\n";
    return os;
}