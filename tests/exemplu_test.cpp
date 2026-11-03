#undef NDEBUG                                                                                                                                                                                       
    #include <cassert>                                                                                                                                                                                  
    #include <iostream>                                                                                                                                                                                 
    #include "mediaFile.h"                                                                                                                                                                              
    #include "playlist.h"                                                                                                                                                                               
    #include "playerSettings.h"                                                                                                                                                                         
    #include "playlistManager.h"                                                                                                                                                                        
                                                                                                                                                                                                        
    int main()                                                                                                                                                                                          
    {                                                                                                                                                                                                   

        mediaFile f1("melodie1", "./media/melodie1.mp3", ".mp3", 120, 2048);
        assert(f1.getFileName() == "melodie1");                                                                                                                                                         
        assert(f1.getFilePath() == "./media/melodie1.mp3");                                                                                                                                             
        assert(f1.getFileType() == ".mp3");                                                                                                                                                             
        assert(f1.getDuration() == 120);                                                                                                                                                                
        assert(f1.getFileSize() == 2048);                                                                                                                                                               
        assert(f1.isAudio());                                                                                                                                                                           
        assert(!f1.isVideo());                                                                                                                                                                          
        assert(f1.formatDuration() == "02:00");                                                                                                                                                         
        assert(f1.calculateBitrate() > 0.0);                                                                                                                                                            
        assert(f1.getResolutionPreset() == "Audio");                                                                                                                                                    
        const bool scanned = f1.scanMedia();
        assert(scanned);
        assert(f1.writeToCatalog("test_temp_catalog.txt"));                                                                                                                                             
                                                                                                                                                                                                        
        mediaFile f2(f1);
        assert(f2.getFileName() == f1.getFileName());                                                                                                                                                   
        assert(f2 == f1);                                                                                                                                                                               
                                                                                                                                                                                                        
        mediaFile f3;                                                                                                                                                                                   
        f3 = f1;
        assert(f3.getDuration() == 120);                                                                                                                                                                
                                                                                                                                                                                                        
        mediaFile video1("video1", "./media/video1.mp4", ".mp4", 300, 10240);                                                                                                                           
        assert(video1.isVideo());                                                                                                                                                                       
        assert(!video1.isAudio());                                                                                                                                                                      
        assert(f1 < video1);
                                                                                                                                                                                                        
        PlayerSettings ps(80, 1.5, 1920, 1080, false);
        assert(ps.getVolume() == 80);                                                                                                                                                                   
        assert(ps.getPlaybackSpeed() == 1.5);                                                                                                                                                           
        assert(ps.getResolutionWidth() == 1920);                                                                                                                                                        
        assert(ps.getResolutionHeight() == 1080);                                                                                                                                                       
        assert(!ps.isFullscreen());                                                                                                                                                                     
                                                                                                                                                                                                        
        ps.adjustVolume(10);                                                                                                                                                                            
        assert(ps.getVolume() == 90);                                                                                                                                                                   
        ps.setVolume(50);                                                                                                                                                                               
        assert(ps.getVolume() == 50);                                                                                                                                                                   
                                                                                                                                                                                                        
        ps.setPlaybackSpeed(2.0);                                                                                                                                                                       
        assert(ps.getPlaybackSpeed() == 2.0);                                                                                                                                                           
        assert(ps.calculateEffectiveDuration(200) == 100);                                                                                                                                              
                                                                                                                                                                                                        
        ps.toggleFullscreen();                                                                                                                                                                          
        assert(ps.isFullscreen());                                                                                                                                                                      
        ps.setFullscreen(false);                                                                                                                                                                        
        assert(!ps.isFullscreen());                                                                                                                                                                     
                                                                                                                                                                                                        
        ps.applyResolutionPreset("HD");                                                                                                                                                                 
        assert(ps.getResolutionPreset() == "HD");                                                                                                                                                       
        assert(ps.getResolutionWidth() == 1280);                                                                                                                                                        
        assert(ps.getResolutionHeight() == 720);                                                                                                                                                        
                                                                                                                                                                                                        
        ps.setCustomResolution(800, 600);                                                                                                                                                               
        assert(ps.getResolutionWidth() == 800);                                                                                                                                                         
        assert(ps.getResolutionHeight() == 600);                                                                                                                                                        
                                                                                                                                                                                                        
        ps.displayPlayerScreen("PiesaTest", "mp3", 100, 20);
        assert(ps.playRay("PiesaTest", "mp3", 10, false));
                                                                                                                                                                                                        
        ps.resetDefaults();                                                                                                                                                                             
        assert(ps.getVolume() == 10);                                                                                                                                                                   
        assert(ps.getPlaybackSpeed() == 1.0);                                                                                                                                                           
                                                                                                                                                                                                        
        Playlist pl("ListaTest", false);
        assert(pl.getName() == "ListaTest");                                                                                                                                                            
        assert(pl.getFileCount() == 0);                                                                                                                                                                 
                                                                                                                                                                                                        
        pl.addFile(f1);                                                                                                                                                                                 
        pl.addFile(video1);                                                                                                                                                                             
        assert(pl.getFileCount() == 2);                                                                                                                                                                 
        assert(pl.containsFile("melodie1"));                                                                                                                                                            
        assert(pl.containsFile("video1"));                                                                                                                                                              
        assert(!pl.containsFile("inexistent"));                                                                                                                                                         
                                                                                                                                                                                                        
        mediaFile nextTrack("piesa_noua", "./media/piesa_noua.mp3", ".mp3", 60, 512);                                                                                                                   
        pl.addFilePlayNext(nextTrack);                                                                                                                                                                  
        assert(pl.getFiles().front().getFileName() == "piesa_noua");                                                                                                                                    
                                                                                                                                                                                                        
        assert(pl.getTotalDuration() == 480);                                                                                                                                                           
        assert(pl.getTotalFileSizeKB() == 12800);                                                                                                                                                       
        assert(pl.getAverageDuration() == 160.0);                                                                                                                                                       
        assert(pl.getLongestFile().getFileName() == "video1");                                                                                                                                          
        assert(pl.getShortestFile().getFileName() == "piesa_noua");                                                                                                                                     
                                                                                                                                                                                                        
        Playlist doarVideo = pl.filterVideosOnly();                                                                                                                                                     
        assert(doarVideo.getFileCount() == 1);                                                                                                                                                          
        Playlist doarAudio = pl.filterAudioOnly();                                                                                                                                                      
        assert(doarAudio.getFileCount() == 2);                                                                                                                                                          
        Playlist cautate = pl.searchByName("video");                                                                                                                                                    
        assert(cautate.getFileCount() == 1);                                                                                                                                                            
                                                                                                                                                                                                        
        pl.sortByDuration(true);                                                                                                                                                                        
        assert(pl.getFiles().front().getDuration() == 300);                                                                                                                                             
                                                                                                                                                                                                        
        pl.swapFiles(0, 1);                                                                                                                                                                             
        const bool rep = pl.toggleRepeatAll();
        assert(rep);
        assert(pl.isRepeatAll() == true);                                                                                                                                                               
                                                                                                                                                                                                        
        pl.playAndRotate();                                                                                                                                                                             
        assert(pl.exportToM3U("test_export.m3u"));                                                                                                                                                      
        assert(pl.exportToCatalog("test_export_catalog.txt"));                                                                                                                                          
                                                                                                                                                                                                        
        const bool removed = pl.removeFile("piesa_noua");
        assert(removed);
        pl.clear();                                                                                                                                                                                     
        assert(pl.getFileCount() == 0);                                                                                                                                                                 
                                                                                                                                                                                                        
        PlaylistManager pm("assets/configFile.config", ps);
        pm.addMediaToLibrary(f1);                                                                                                                                                                       
        pm.addMediaToLibrary(video1);                                                                                                                                                                   
        assert(pm.getMediaList().size() >= 2);                                                                                                                                                          
                                                                                                                                                                                                        
        pm.addFavouriteByName("melodie1");                                                                                                                                                              
        assert(pm.getFavouritePlaylist().containsFile("melodie1"));                                                                                                                                     
        const bool favRemoved = pm.removeFavouriteByName("melodie1");
        assert(favRemoved);
                                                                                                                                                                                                        
        assert(pm.getQueueCount() >= 2);                                                                                                                                                                
        pm.playNextInQueue();                                                                                                                                                                           
        pm.clearQueue();                                                                                                                                                                                
        assert(pm.getQueueCount() == 0);                                                                                                                                                                
                                                                                                                                                                                                        
        assert(pm.getEffectiveLibraryDuration() > 0);                                                                                                                                                   
        assert(pm.getLibraryTotalSizeKB() > 0);                                                                                                                                                         
        assert(!pm.searchLibrary("melodie").empty());                                                                                                                                                   
        assert(!pm.filterByCodec("mp3").empty());                                                                                                                                                       
                                                                                                                                                                                                        
        pm.updateSettings(ps);                                                                                                                                                                          
        assert(pm.saveConfig("test_saved_config.config"));                                                                                                                                              
        pm.updateFolderPath("./media");                                                                                                                                                                 
        assert(pm.getFolderPath() == "./media");                                                                                                                                                        
        assert(pm.exportLibraryCatalog("test_library_catalog.txt"));                                                                                                                                    
                                                                                                                                                                                                        
        std::cout << "Toate testele au fost executate cu succes!\n";                                                                                                                                    
        return 0;                                                                                                                                                                                       
    }