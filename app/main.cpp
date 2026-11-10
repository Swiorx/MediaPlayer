#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "mediaFile.h"
#include "playlist.h"
#include "playerSettings.h"
#include "playlistManager.h"
#include "mediaDecoder.h"

int main()
{
    std::string configPath = "assets/configFile.config";
    PlaylistManager manager(configPath);
    PlayerSettings setari = manager.getSettings();

    std::string catalogPath = "assets/catalog.txt";
    std::ifstream catalogIn(catalogPath);
    Playlist playlistPrincipal("Mix", true);
    std::string catalogLine;
    while (std::getline(catalogIn, catalogLine))
    {
        std::istringstream iss(catalogLine);
        std::string name, path, type;
        int duration = 0;
        int size = 0;
        if (iss >> name >> path >> type >> duration >> size)
        {
            mediaFile readFile(name, path, type, duration, size);
            readFile.scanMedia();
            playlistPrincipal.addFile(readFile);
            manager.addMediaToLibrary(readFile);
        }
    }
    catalogIn.close();

    std::cout << "---------------------------------------------------------\n";
    std::cout << "Comenzi interactive acceptate:\n";
    std::cout << "  1 Modificare setari si salvare in configFile.config\n";
    std::cout << "  2 Adaugare melodie la favorite\n";
    std::cout << "  3 Stergere melodie din playlist / favorite\n";
    std::cout << "  4 Reda fisier din biblioteca sau coada\n";
    std::cout << "  5 Adaugare melodie noua in catalog si scriere in catalog.txt\n";
    std::cout << "  6 Creare playlist nou din melodii si export M3U\n";
    std::cout << "  7 Sortare playlist si playAndRotate\n";
    std::cout << "  8 Filtrare fluxuri si afisare statistici\n";
    std::cout << "  9 Afisare sumar biblioteca si configuratie\n";
    std::cout << "  0 Iesire curata din aplicatie\n";
    std::cout << "---------------------------------------------------------\n";

    int optiune = 0;
    while (true)
    {
        std::cout << "\nAlegeti optiunea dorita (0-9): ";
        if (!(std::cin >> optiune) || optiune == 0)
        {
            break;
        }

        if (optiune == 1)
        {
            int deltaVol = 5;
            double vitezaNoua = 1.0;
            std::string presetNou;
            std::cout << "Introduceti deltaVol, vitezaNoua, presetNou (ex: 5 1.0 HD): ";
            if (std::cin >> deltaVol >> vitezaNoua >> presetNou)
            {
                setari.adjustVolume(deltaVol);
                setari.setVolume(setari.getVolume());
                setari.setPlaybackSpeed(vitezaNoua);
                setari.applyResolutionPreset(presetNou);
                setari.setCustomResolution(setari.getResolutionWidth(), setari.getResolutionHeight());
                setari.toggleFullscreen();
                setari.setFullscreen(!setari.isFullscreen());
                setari.setFullscreen(false);
                manager.updateSettings(setari);
                manager.saveConfig(configPath);
                std::cout << "Setari noi aplicate si salvate in configFile: " << manager.getSettings();
                std::cout << "Preset: " << setari.getResolutionPreset()
                          << " | Volum: " << setari.getVolume()
                          << " | Viteza: " << setari.getPlaybackSpeed()
                          << " | Rezolutie: " << setari.getResolutionWidth() << "x" << setari.getResolutionHeight()
                          << " | Fullscreen: " << setari.isFullscreen() << "\n";
            }
        }
        else if (optiune == 2)
        {
            std::string numeFav;
            std::cout << "Introduceti numele melodiei pentru favorite: ";
            if (std::cin >> numeFav)
            {
                manager.addFavouriteByName(numeFav);
                std::cout << "Adaugat la favorite. Playlist favorite curent:\n" << manager.getFavouritePlaylist() << "\n";
            }
        }
        else if (optiune == 3)
        {
            std::string numeStergere;
            std::cout << "Introduceti numele melodiei de sters: ";
            if (std::cin >> numeStergere)
            {
                bool contained = playlistPrincipal.containsFile(numeStergere);
                bool sters = playlistPrincipal.removeFile(numeStergere);
                bool stersFav = manager.removeFavouriteByName(numeStergere);
                std::cout << "Verificare existenta ('" << numeStergere << "'): " << (contained ? "Gasit" : "Nu exista") << "\n";
                std::cout << "Stergere fisier '" << numeStergere << "': "
                          << ((sters || stersFav) ? "Succes" : "Nu a fost gasit") << "\n";
            }
        }
        else if (optiune == 4)
        {
            mediaFile current;
            std::cout << "\nFisiere disponibile in biblioteca:\n";
            const auto &lib = manager.getMediaList();
            for (size_t idx = 0; idx < lib.size(); ++idx)
            {
                std::cout << "  [" << (idx + 1) << "] " << lib[idx].getFileName() << lib[idx].getFileType()
                          << " (" << lib[idx].getCodecName() << ")\n";
            }
            std::cout << "Introduceti numarul [1-" << lib.size() << "] sau 'next': ";
            std::string alegere;
            if (std::cin >> alegere)
            {
                if (alegere == "next" || alegere == "0")
                {
                    current = manager.playNextInQueue();
                }
                else
                {
                    for (size_t idx = 0; idx < lib.size(); ++idx)
                    {
                        if (alegere == std::to_string(idx + 1))
                        {
                            current = lib[idx];
                            break;
                        }
                    }
                }
            }

            int effDur = setari.calculateEffectiveDuration(current.getDuration());
            setari.displayPlayerScreen(current.getFileName(), current.getCodecName(), current.getDuration(), 0, std::cout);
            std::cout << "Durata la viteza curenta: " << effDur << "s | Elemente ramase in coada: " << manager.getQueueCount() << "\n";

            if (current.isVideo() && !current.getFilePath().empty())
            {
                MediaDecoder decoder;
                decoder.open(current.getFilePath());
                std::cout << "Status decodor: " << decoder;
                if (decoder.hasCurrentFrame())
                {
                    std::cout << "Dimensiuni video: " << decoder.getVideoWidth() << "x" << decoder.getVideoHeight()
                              << " la " << decoder.getVideoFps() << " FPS. Sample progres: " << decoder.getNextVideoSampleIdx()
                              << "/" << decoder.getTotalVideoSamples() << " (" << decoder.getPlaybackProgress() * 100.0f << "%)\n";
                    std::cout << "Buffer frame RGBA: " << static_cast<const void*>(decoder.getFrameData())
                              << " (" << decoder.getCurrentFrameRGBA().size() << " bytes)\n";
                    decoder.seekToRatio(0.1f);
                    decoder.decodeNextVideoFrame();
                }
                if (decoder.isAacReady())
                {
                    std::cout << "Audio AAC: " << decoder.getAudioSampleRate() << " Hz, " << decoder.getAudioChannels() << " canale.\n";
                    int16_t dummyPcm[64];
                    decoder.fillAudioStreamChunk(dummyPcm, 32);
                }
            }

            if (!current.getFileName().empty() && current.getFileName() != "unknown")
            {
                setari.playRay(current.getFileName() + current.getFileType(), current.getCodecName(), current.getDuration(), true, current.getFilePath());
            }
        }
        else if (optiune == 5)
        {
            std::string nume, cale, tip;
            int durata = 0;
            int marime = 0;
            std::cout << "Introduceti nume cale tip durata marime (ex: melodie ./media/piesa.mp3 .mp3 180 5000): ";
            if (std::cin >> nume >> cale >> tip >> durata >> marime)
            {
                mediaFile melodieNoua(nume, cale, tip, durata, marime);
                melodieNoua.scanMedia();
                std::cout << "Fisier analizat: " << melodieNoua.getFileName() << melodieNoua.getFileType()
                          << " | Durata: " << melodieNoua.formatDuration()
                          << " | Marime: " << melodieNoua.formatFileSize() << " (" << melodieNoua.getFileSize() << " KB)"
                          << " | Bitrate: " << melodieNoua.calculateBitrate() << " kbps"
                          << " | Tip: " << (melodieNoua.isAudio() ? "Audio" : (melodieNoua.isVideo() ? "Video" : "Alt format")) << "\n";

                if (!playlistPrincipal.getFiles().empty())
                {
                    const mediaFile &first = playlistPrincipal.getFiles().front();
                    bool esteEgal = (melodieNoua == first);
                    bool esteMaiMic = (melodieNoua < first);
                    std::cout << "Comparatie cu prima piesa (" << first.getFileName() << "): "
                              << "Egal=" << esteEgal << ", MaiMic=" << esteMaiMic << "\n";
                }

                playlistPrincipal.addFile(melodieNoua);
                playlistPrincipal.addFilePlayNext(melodieNoua);
                manager.addMediaToLibrary(melodieNoua);

                bool scris = melodieNoua.writeToCatalog(catalogPath);
                std::cout << "Melodie adaugata si salvata in " << catalogPath << ": " << (scris ? "Succes" : "Eroare scriere") << "\n";
                melodieNoua.printStreamInfo(std::cout);
            }
        }
        else if (optiune == 6)
        {
            std::string numePlaylist, keyword;
            std::cout << "Introduceti numePlaylist keyword (ex: PlaylistRock rock): ";
            if (std::cin >> numePlaylist >> keyword)
            {
                Playlist playlistNou = playlistPrincipal.searchByName(keyword);
                std::string m3uPath = numePlaylist + ".m3u";
                playlistNou.exportToM3U(m3uPath);
                std::cout << "Playlist '" << playlistNou.getName() << "' creat cu " << playlistNou.getFileCount()
                          << " melodii (" << playlistNou.getTotalFileSizeKB() << " KB), Repeat=" << playlistNou.isRepeatAll() << ".\n";
                std::cout << playlistNou << "\n";
            }
        }
        else if (optiune == 7)
        {
            std::cout << "Playlist curent: " << playlistPrincipal.getName() << " (" << playlistPrincipal.getFiles().size()
                      << " piese), Repeat=" << playlistPrincipal.isRepeatAll() << "\n";
            playlistPrincipal.toggleRepeatAll();
            playlistPrincipal.sortByDuration(false);
            playlistPrincipal.playAndRotate();
            std::cout << "Stare playlist dupa sortare si rotire:\n" << playlistPrincipal << "\n";
        }
        else if (optiune == 8)
        {
            Playlist audioPl = playlistPrincipal.filterAudioOnly();
            Playlist videoPl = playlistPrincipal.filterVideosOnly();
            std::cout << "Fisiere Audio: " << audioPl.getFileCount() << " (Durata totala: " << audioPl.getTotalDuration() << "s)\n";
            std::cout << "Fisiere Video: " << videoPl.getFileCount() << " (Durata totala: " << videoPl.getTotalDuration() << "s)\n";
        }
        else if (optiune == 9)
        {
            manager.updateFolderPath("./media");
            manager.updateMediaList();
            std::cout << "Config Path: " << configPath << " | Folder Scanat: " << manager.getFolderPath() << "\n";
            std::cout << "Setari curente: " << manager.getSettings();
            std::cout << "Durata efectiva a bibliotecii: " << manager.getEffectiveLibraryDuration()
                      << "s | Spatiu total: " << manager.getLibraryTotalSizeKB()
                      << " KB | Coada: " << manager.getQueueCount() << "\n";
            manager.printLibrarySummary(std::cout);
        }
        else
        {
            std::cout << "Optiune nerecunoscuta: " << optiune << "\n";
        }
    }

    std::cout << "\nStare finala PlaylistManager:\n" << manager << "\n";

    // Curatare si resetare pentru a demonstra operatiile de gestiune
    PlayerSettings defaultReset;
    defaultReset.resetDefaults();
    Playlist emptyDemo("TempDemo");
    emptyDemo.addFile(mediaFile("temp", "./media/temp.mp3", ".mp3", 10, 100));
    emptyDemo.clear();
    manager.clearQueue();
    manager.loadDefaultConfig();

    return 0;
}