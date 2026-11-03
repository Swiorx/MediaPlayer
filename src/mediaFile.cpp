#include "mediaFile.h"
#include "minimp4.h"
#include "mkvparser/mkvparser.h"
#include "mkvparser/mkvreader.h"
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <cstring>
#include <cstdio>

bool mediaFile::validateDurationSize() const
{
    return (*duration > 0 && *fileSize > 0);
}

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

struct mp4MemoryBuffer
{
    const uint8_t *data;
    int64_t size;
};

static int minimp4Read(int64_t offset, void *buffer, size_t size, void *token)
{
    const auto *buf = static_cast<const mp4MemoryBuffer*>(token);
    if (!buf || offset < 0 || offset >= buf->size)
    {
        return -1;
    }

    size_t toCopy = static_cast<size_t>(buf->size - offset);
    if (toCopy > size)
    {
        toCopy = size;
    }
    std::memcpy(buffer, buf->data + offset, toCopy);
    return (toCopy == size) ? 0 : -1;
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
        FILE *f = std::fopen(path.c_str(), "rb");
        if (f)
        {
            std::fseek(f, 0, SEEK_END);
            long fSize = std::ftell(f);
            std::fseek(f, 0, SEEK_SET);

            if (fSize > 16)
            {
                std::vector<uint8_t> buffer(static_cast<size_t>(fSize));
                size_t readCount = std::fread(buffer.data(), 1, buffer.size(), f);
                std::fclose(f);
                f = nullptr;

                mp4MemoryBuffer mp4Buf = { buffer.data(), static_cast<int64_t>(readCount) };
                MP4D_demux_t mp4;
                std::memset(&mp4, 0, sizeof(mp4));

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
                            if (mp4.track[i].object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3 || mp4.track[i].object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_LC_PROFILE)
                            {
                                hasAAC = true;
                            }
                        }
                    }
                    if (hasVideo)
                    {
                        codecName = hasAAC ? "h264+aac" : "h264";
                    }
                    else if (hasAAC)
                    {
                        codecName = "aac";
                    }
                    else
                    {
                        codecName = "h264";
                    }
                    MP4D_close(&mp4);
                    return true;
                }
            }
            if (f)
            {
                std::fclose(f);
            }
        }
        codecName = "h264";
        return true;
    }
    if (fileType == ".mkv")
    {
        mkvparser::MkvReader reader;
        if (reader.Open(path.c_str()) == 0)
        {
            long long pos = 0;
            mkvparser::Segment *pSegment = nullptr;
            long long status = mkvparser::Segment::CreateInstance(&reader, pos, pSegment);
            if (status == 0 && pSegment != nullptr)
            {
                if (pSegment->Load() >= 0)
                {
                    long long durationNs = pSegment->GetDuration();
                    if (durationNs > 0)
                    {
                        *duration = static_cast<int>(durationNs / 1000000000LL);
                    }
                    const mkvparser::Tracks *pTracks = pSegment->GetTracks();
                    if (pTracks != nullptr)
                    {
                        for (unsigned int i = 0; i < pTracks->GetTracksCount(); i++)
                        {
                            const mkvparser::Track *pTrack = pTracks->GetTrackByIndex(i);
                            if (pTrack != nullptr && pTrack->GetType() == mkvparser::Track::kVideo)
                            {
                                const char *cId = pTrack->GetCodecId();
                                codecName = cId ? std::string(cId) : "V_MPEG4/ISO/AVC";
                                break;
                            }
                        }
                    }
                }
                delete pSegment;
                reader.Close();
                if (codecName == "unscanned")
                {
                    codecName = "h264/matroska";
                }
                return true;
            }
            reader.Close();
        }

        codecName = "h264/matroska";
        return true;
    }

    codecName = "nespecificat";
    return false;
}

void mediaFile::printStreamInfo(std::ostream &os) const
{
    os<<"Detalii stream pentru: " << fileName << fileType << "\n";
    os<<"Cale: " << (filePath.empty() ? "nespecificata" : filePath) << "\n";
    os << "Tip fisier: " << fileType << "\n";
    os << "Codec detectat: " << codecName << "\n";
    os << "Durata: " << formatDuration() << " " << *duration << " secunde" << "\n";
    os << "Marime fisier: " << formatFileSize() << " " << *fileSize << " KB" "\n";
    os << "Bitrate calculat: " << std::fixed << std::setprecision(2) << calculateBitrate() << " kbps" << "\n";
    if (isVideo())
    {
        os << "Tip media: " << (fileType == ".mp4" ? "minimp4" : "mkvparser") << "\n";
        os<< "Preset rezolutie: " << getResolutionPreset() << "\n";
    }
    else if (isAudio())
    {
        os << "Tip media: Audio" << "\n";
    }
    else
    {
        os << "Tip media: Nespecificat" << "\n";
    }
}

bool mediaFile::writeToCatalog(const std::string& catalogPath) const
{
    std::ofstream out(catalogPath, std::ios::app);
    if (!out.is_open()) return false;
    out << fileName << " " << (filePath.empty() ? "./media" + fileName + fileType : filePath) << " " << fileType << " " << getDuration() << " " << getFileSize() << "\n";
    return out.good();
}


bool mediaFile::validateFileIntegrity() const
{
    if (fileName.empty() || fileType.empty())
    {
        return false;
    }
    if (*duration <= 0 || *fileSize <= 0)
    {
        return false;
    }
    if (!filePath.empty())
    {
        return std::filesystem::exists(filePath);
    }
    return true;
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

std::string mediaFile::getResolutionPreset() const
{
    if (!isVideo())
    {
        return "Audio";
    }
    if (*fileSize >= 100000 || calculateBitrate() >= 5000.0)
    {
        return "4K";
    }
    if (*fileSize >= 20000 ||calculateBitrate() >= 1500.0)
    {
        return "FHD";
    }
    return "HD";
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