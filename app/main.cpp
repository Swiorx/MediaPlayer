#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "mediaFile.h"
#include "playlist.h"
#include "playerSettings.h"
#include "playlistManager.h"

int main()
{
    std::ifstream tastaturaText;
    tastaturaText.open("tastatura.txt");
    if (tastaturaText.is_open())
    {
        std::cout << "Tastatura.txt a fost citit" << "\n";
    }

    std::streambuf *origCin = nullptr;
    if (tastaturaText.is_open())
    {
        origCin = std::cin.rdbuf(tastaturaText.rdbuf());
    }

    std::cout<< "Se incarca setarile din configFile";
    std::string configPath = "assets/configFile.config";

    PlaylistManager manager(configPath);
    PlayerSettings setari = manager.getSettings();

    setari.initAudioEngine();

    std::cout<<"Folder de scanat pentru media incarcat din config: " << manager.getFolderPath() << "\n";
    std::cout << "Setari incarcate din config: " << setari << "\n";
    std::cout << "Descriere extinsa: " << setari.getSettingsDescription() << "\n";
    std::cout << "Aspect ratio: " << setari.getAspectRatio() << "\n";
    std::cout << "Volum: " << setari.getVolume() << "\n";
    std::cout << "Viteza de playback: " << setari.getPlaybackSpeed() << "\n";
    std::cout << "Rezolutie: " << setari.getResolutionWidth() << "x" << setari.getResolutionHeight() << "\n";
    std::cout << "Fullscreen: " << setari.isFullscreen() << "\n";
    std::cout << "Durata efectia a 180s la viteza curenta: " << setari.calculateEffectiveDuration(180) << "\n";

    setari.setVolume(setari.getVolume());
    setari.setFullscreen(false);
    setari.toggleFullscreen();
    setari.toggleFullscreen();
    setari.setCustomResolution(1920, 1080);
    std::cout << "Preset rezolutie setari: " << setari.getResolutionPreset() << "\n";
    setari.playRay("DemoTest", "mp3", 10, false);

    std::cout << "Incarcare melodii din catalog.txt si playlist" << "\n";
    std::string catalogPath = "assets/catalog.txt";
    std::ifstream catalogIn;
    catalogIn.open(catalogPath);
    if (!catalogIn.is_open())
    {
        std::cerr<<"Nu s-a putut deschide fisierul" << "\n";
    }

    Playlist playlistDefault;
    std::cout<< "Playlist default initializat, elemente: " << playlistDefault.getFileCount() << "\n";

    PlayerSettings setariImplicite;
    setariImplicite.resetDefaults();
    std::cout << "Setari resetate la default. Volum: " << setariImplicite.getVolume() << "\n";

    Playlist playlistPrincipal("Mix", true);
    std::string catalogLine;
    while (std::getline(catalogIn, catalogLine))
    {
        if (catalogLine.empty()){}
        std::istringstream iss(catalogLine);
        std::string name,path,type;
        int duration = 0;
        int size = 0;
        if (iss>>name>>path>>type>>duration>>size)
        {
            mediaFile readFile(name,path,type,duration,size);
            readFile.scanMedia();
            playlistPrincipal.addFile(readFile);
        }
    }
    catalogIn.close();

    std::cout << "S-au incarcat: " << playlistPrincipal.getFileCount() << " melodii in playlistPrincipal" << "\n";

    if (!playlistPrincipal.getFiles().empty())
    {
        const mediaFile &first = playlistPrincipal.getFiles().front();
        mediaFile constructorCopy(first);
        mediaFile assignmentCopy;
        assignmentCopy = constructorCopy;
        constructorCopy = constructorCopy;

        std::cout << "Fisier copiat prin operator<<: " << assignmentCopy << "\n";
        assignmentCopy.printStreamInfo(std::cout);
        assignmentCopy.scan();
        std::cout << "Validare integritate fisier: " << (assignmentCopy.validateFileIntegrity() ? "Da" : "Nu") << "\n";
        std::cout << "Marime: " << assignmentCopy.formatFileSize() << "Preset rezolutie: " << assignmentCopy.getResolutionPreset() << "\n";
        std::cout << "Test operator== :" << (assignmentCopy == first) << " Test operator< :" << (assignmentCopy < first) << "\n";

        std::cout << "Nume: " << assignmentCopy.getFileName() << "Cale: " << assignmentCopy.getFilePath() << "Tip: " << assignmentCopy.getFileType() << "Durata: " << assignmentCopy.formatDuration() << "Marime: " << assignmentCopy.getFileSize() << "Bitrate: " <<assignmentCopy.calculateBitrate() << "Codec: " << assignmentCopy.getCodecName() << "Video: " << assignmentCopy.isVideo() << "Audio: " << assignmentCopy.isAudio() << "\n";

        playlistPrincipal.addFile(assignmentCopy);
    }

    std::cout << playlistPrincipal.getName() << "\n";
    std::cout << "Repeat: " << playlistPrincipal.isRepeatAll() << "\n";
    std::cout << "Durata totala: " << playlistPrincipal.getTotalDuration() << "\n";
    std::cout << "Marime totala: " << playlistPrincipal.getTotalFileSizeKB() << "\n";
    std::cout << "Durata medie: " << playlistPrincipal.getAverageDuration() << "\n";
    std::cout << playlistPrincipal << "\n";

    std::cout << "Sortare descrescator dupa durata: " << "\n";
    playlistPrincipal.sortByDuration(true);
    std::cout << playlistPrincipal << "\n";

    std::cout << "Exista 'melodie_house' in playlist?: " << playlistPrincipal.containsFile("melodie_house") << "\n";
    std::cout << "Cea mai lunga piesa: " << playlistPrincipal.getLongestFile().getFileName() << "\n";
    std::cout << "Cea mai scurta piesa: " << playlistPrincipal.getShortestFile().getFileName() << "\n";
    playlistPrincipal.toggleRepeatAll();
    playlistPrincipal.toggleRepeatAll();

    Playlist doarVideo = playlistPrincipal.filterVideosOnly();
    Playlist doarAudio = playlistPrincipal.filterAudioOnly();
    Playlist cautare = playlistPrincipal.searchByName("rock");
    std::cout << "Filtru video: " << doarVideo << "\n";
    std::cout << "Filtru audio: " << doarAudio << "\n";
    std::cout << "Cautare rock: " << cautare << "\n";

    playlistPrincipal.swapFiles(0,1);
    playlistPrincipal.exportToM3U("playlistPrincipal.m3u");
    playlistPrincipal.exportToCatalog("catalog_export.txt");

    Playlist tempPlaylist = playlistPrincipal;
    std::cout << "Playlist temporar inainte de clear: " << tempPlaylist.getFileCount() << "\n";
    tempPlaylist.clear();
    std::cout << "Playlist temporar dupa clear: " << tempPlaylist.getFileCount() << "\n";

    std::cout << "Play and rotate: " << "\n";
    playlistPrincipal.playAndRotate();
    std::cout << "\n";

    std::cout << "Scanare biblioteca " << manager.getFolderPath() << "\n";

    manager.loadDefaultConfig();
    manager.saveConfig();
    manager.updateFolderPath("./media");
    manager.clearQueue();
    manager.exportLibraryCatalog("catalog_manager_export.txt");
    manager.updateMediaList();

    for (const auto &f : playlistPrincipal.getFiles())
    {
        manager.addMediaToLibrary(f);
    }

    std::vector<mediaFile> gasite = manager.searchLibrary("house");
    std::vector<mediaFile> h264 = manager.filterByCodec("h264");
    std::cout<< "Cautare biblioteca dupa 'house':" << gasite.size() << " Fisiere cu codec h264: " << h264.size() << "\n";
    std::cout << "Dimensiune totala biblioteca: " << manager.getLibraryTotalSizeKB() << "KB" << "\n";
    std::cout << "Elemente in queue: " << manager.getQueueCount() << "\n";

    manager.printLibrarySummary(std::cout);
    std::cout << "Durata efectiva la viteza stabilita a bibliotecii: " << manager.getEffectiveLibraryDuration() << "\n";

    std::cout << "---------------------------------------------------------\n";
    std::cout << "Comenzi interactive acceptate:\n";
    std::cout << "  1 Modificare setari si salvare in configFile.config\n";
    std::cout << "  2 Adaugare melodie la favorite\n";
    std::cout << "  3 Stergere melodie din playlist / favorite\n";
    std::cout << "  4 Reda urmatorul fisier din coada\n";
    std::cout << "  5 Adaugare melodie noua in catalog si scriere in catalog.txt\n";
    std::cout << "  6 Creare playlist nou din melodii si export\n";
    std::cout << "  7 Sortare playlist si playAndRotate\n";
    std::cout << "  8 Filtrare fluxurisi afisare statistici\n";
    std::cout << "  9 Afisare sumar biblioteca si configuratie\n";
    std::cout << "  0 Iesire curata din aplicatie\n";
    std::cout << "---------------------------------------------------------\n";

    int optiune = 0;
    while (std::cin >> optiune && optiune != 0)
    {
        if (optiune == 1)
        {
            int deltaVol = 5;
            double vitezaNoua = 1.0;
            std::string presetNou;
            if (std::cin >> deltaVol >> vitezaNoua >> presetNou)
            {
                std::cout << "Modificare setari deltaVol=" << deltaVol<< ", viteza=" << vitezaNoua << ", preset=" << presetNou << ")...\n";
                setari.adjustVolume(deltaVol);
                setari.setPlaybackSpeed(vitezaNoua);
                setari.applyResolutionPreset(presetNou);
                manager.updateSettings(setari);
                std::cout << "Setari noi aplicate si salvate in configFile: " << manager.getSettings();
            }
        }
        else if (optiune == 2)
        {
            std::string numeFav;
            if (std::cin >> numeFav)
            {
                std::cout << "Adaugare melodie '" << numeFav << "' la favorite\n";
                manager.addFavouriteByName(numeFav);
                std::cout << "Adaugat la favorite. Playlist favorite curent:\n" << manager.getFavouritePlaylist() << "\n";
            }
        }
        else if (optiune == 3)
        {
            std::string numeStergere;
            if (std::cin >> numeStergere)
            {
                std::cout << "Stergere melodie '" << numeStergere << "'\n";
                bool sters = playlistPrincipal.removeFile(numeStergere);
                bool stersFav = manager.removeFavouriteByName(numeStergere);
                std::cout << "Stergere fisier '" << numeStergere << "': "
                          << ((sters || stersFav) ? "Succes" : "Nu a fost gasit") << "\n";
            }
        }
        else if (optiune == 4)
        {
            std::cout << "Redare urmatorul fisier din coada:\n";
            manager.playNextInQueue();
        }
        else if (optiune == 5)
        {
            std::string nume, cale, tip;
            int durata = 0;
            int marime = 0;
            if (std::cin >> nume >> cale >> tip >> durata >> marime)
            {
                std::cout << "Adaugare melodie noua '" << nume << "' si scriere in catalog.txt\n";
                mediaFile melodieNoua(nume, cale, tip, durata, marime);
                melodieNoua.scanMedia();
                playlistPrincipal.addFile(melodieNoua);
                playlistPrincipal.addFilePlayNext(melodieNoua);
                manager.addMediaToLibrary(melodieNoua);

                // Scriere in catalog.txt
                bool scris = melodieNoua.writeToCatalog(catalogPath);
                if (catalogPath != "catalog.txt")
                {
                    melodieNoua.writeToCatalog("catalog.txt");
                }
                std::cout << "Melodie adaugata si salvata in " << catalogPath<< ": " << (scris ? "Succes" : "Eroare scriere") << "\n";
                std::cout << "Detalii melodie noua: " << melodieNoua;
            }
        }
        else if (optiune == 6)
        {
            std::string numePlaylist, keyword;
            if (std::cin >> numePlaylist >> keyword)
            {
                std::cout << "Creare playlist nou '" << numePlaylist<< "' filtrat dupa '" << keyword << "' si export...\n";
                Playlist playlistNou = playlistPrincipal.searchByName(keyword);
                std::string m3uPath = numePlaylist + ".m3u";
                std::string catPath = numePlaylist + "_catalog.txt";
                playlistNou.exportToM3U(m3uPath);
                playlistNou.exportToCatalog(catPath);
                std::cout << "Playlist '" << numePlaylist << "' creat cu "<< playlistNou.getFileCount() << " melodii. Exportat in " << m3uPath<< " si " << catPath << "\n";
                std::cout << playlistNou << "\n";
            }
        }
        else if (optiune == 7)
        {
            std::cout << "Sortare playlist si rotire coada (playAndRotate)\n";
            playlistPrincipal.sortByDuration(false);
            playlistPrincipal.playAndRotate();
            std::cout << "Stare playlist dupa sortare si rotire:\n" << playlistPrincipal << "\n";
        }
        else if (optiune == 8)
        {
            std::cout << "Filtrare fluxuri\n";
            Playlist audioPl = playlistPrincipal.filterAudioOnly();
            Playlist videoPl = playlistPrincipal.filterVideosOnly();
            std::cout << "Fisiere Audio: " << audioPl.getFileCount() << " (Durata totala: " << audioPl.getTotalDuration() << "s)\n";
            std::cout << "Fisiere Video: " << videoPl.getFileCount() << " (Durata totala: " << videoPl.getTotalDuration() << "s)\n";
        }
        else if (optiune == 9)
        {
            std::cout << "Afisare sumar complet biblioteca si configuratie\n";
            std::cout << "Config Path: " << configPath << "\n";
            std::cout << "Folder Scanat: " << manager.getFolderPath() << "\n";
            std::cout << "Setari curente: " << manager.getSettings();
            manager.printLibrarySummary(std::cout);
        }
        else
        {
            std::cout << "Optiune nerecunoscuta: " << optiune << "\n";
        }
    }

    std::cout << " Iesire din meniul interactiv\n";

    std::cout << "\nStare finala PlaylistManager:\n";
    std::cout << "Durata efectiva biblioteca: " << manager.getEffectiveLibraryDuration() << "s" << " Fisiere: " << manager.getMediaList().size()<< " Cale folder scanat: " << manager.getFolderPath() << "\n";
    std::cout << manager << "\n";

    // Restaurare stream std::cin
    if (origCin != nullptr)
    {
        std::cin.rdbuf(origCin);
    }
}