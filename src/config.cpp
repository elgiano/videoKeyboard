#include "config.h"

std::vector<SourceGroup> Config::findConfig(){
  std::vector<SourceGroup> sourceGroups;
  ofDirectory dir(ofToDataPath(""));
  // config file
  dir.allowExt("json");
  dir.listDir();
  dir.sort();

  loadDefaultConfig();
  if(dir.size()>0){
    cout << "custom conf" << endl;
    sourceGroups = loadConfig(dir.getPath(0));
  }
  return sourceGroups;
}

  void Config::loadDefaultConfig(){
    cout << "default midi mappings" << endl;
    loadDefaultMidiMappings();
    cout << "default conf" << endl;
    loadConfig(ofToDataPath("../../defaultConf.json"));
  }

  std::vector<SourceGroup>  Config::loadConfig(string path){
    std::vector<SourceGroup> sourceGroups;
    Settings::get().load(path);
    string enclosingDir = ofFilePath::getEnclosingDirectory(path);


    // LAYOUTS
    std::map<int,int> confNumToStoredLayoutConf;
    if(Settings::get().exists("layouts")){
      // count folders, init layoutConf

      //if(n_layoutConfs == 0){;
      int thisFolder = 0;
      int thisConfLayouts = 0;
      while(Settings::get().exists("layouts/"+std::to_string(thisFolder++))){
        thisConfLayouts++;
      }
      //layoutConf = new int*[n_layoutConfs];
      cout << ofToString(thisConfLayouts) <<" layout groups" << endl;

      // for each, split the data in N_LAYOUT numbers
      for(thisFolder = 0; thisFolder < thisConfLayouts; thisFolder++){
        std::vector<string> positions = ofSplitString(Settings::getString("layouts/"+std::to_string(thisFolder)),",");
        //layoutConf.push_back(new int[N_LAYOUTS]);
        std::array<int,N_LAYOUTS-1> thisLayout;
        for(unsigned j=0;j<positions.size() && j<(N_LAYOUTS-1);j++){
          thisLayout[j] = std::stoi(positions[j]);
        }

        // add layout to storage only if it's not already there
        if(std::find(layoutConf.begin(), layoutConf.end(), thisLayout) == layoutConf.end()) {
          layoutConf.push_back(thisLayout);
        }
        confNumToStoredLayoutConf[thisFolder] = std::distance(layoutConf.begin(),std::find(layoutConf.begin(), layoutConf.end(), thisLayout));

      }

      n_layoutConfs = layoutConf.size();
    }

    // SOURCES

    if(Settings::get().exists("sources")){
      int thisFolder = 0;
      // count source groups
      n_sources = 0;
      int thisGroup = 0;
      while(Settings::get().exists("sources/"+std::to_string(thisGroup++))){
        n_sources++;
      }
      sourceGroups = std::vector<SourceGroup>(n_sources);

      while(Settings::get().exists("sources/"+std::to_string(thisFolder))){

        short thisLayout=0;
        short thisDeviceID=0;
        int thisRandSize = 1;
        bool thisIsCapture = false;
        bool thisIsRandom = false;
        bool thisIsMultiple = false;
        bool thisAutoplay = false;

        string thisSrc = "";
        //type, source, layout
        if(Settings::get().exists("sources/"+std::to_string(thisFolder)+"/type")){
            if(Settings::getString("sources/"+std::to_string(thisFolder)+"/type")=="capture"){
              thisIsCapture = true;
            }else if(Settings::getString("sources/"+std::to_string(thisFolder)+"/type")=="random"){
              thisIsRandom = true;
            }else if(Settings::getString("sources/"+std::to_string(thisFolder)+"/type")=="multiple"){
              thisIsMultiple = true;
            }
        }

        if(Settings::get().exists("sources/"+std::to_string(thisFolder)+"/src")){
          if(thisIsCapture){
            thisDeviceID = Settings::getInt("sources/"+std::to_string(thisFolder)+"/src");
          }else{
            thisSrc = Settings::getString("sources/"+std::to_string(thisFolder)+"/src");
          }
        }

        if(Settings::get().exists("sources/"+std::to_string(thisFolder)+"/layout")){
          thisLayout = Settings::getInt("sources/"+std::to_string(thisFolder)+"/layout");
          if(confNumToStoredLayoutConf.find(thisLayout)!=confNumToStoredLayoutConf.end()){
            thisLayout = confNumToStoredLayoutConf[thisLayout];
          }
        }

        if(Settings::get().exists("sources/"+std::to_string(thisFolder)+"/size")){
            thisRandSize = Settings::getInt("sources/"+std::to_string(thisFolder)+"/size");
        }

        if(Settings::get().exists("sources/"+std::to_string(thisFolder)+"/autoplay")){
            thisAutoplay = Settings::getInt("sources/"+std::to_string(thisFolder)+"/autoplay")>0;
        }

        // register source group
        SourceGroup thisSourceGroup{
          (short)(thisIsCapture?1:thisIsRandom?2:thisIsMultiple?3:0),
          thisLayout,
          thisDeviceID,
          thisRandSize,
          thisAutoplay,
          (thisIsMultiple || thisIsCapture) ? thisSrc : ofFilePath::join(enclosingDir,thisSrc)
        };
        sourceGroups[thisFolder] = thisSourceGroup;
        cout << "source group #"<< ofToString(thisFolder)<< " capture:" << ofToString(thisIsCapture) << " random:"<<ofToString(thisIsRandom) <<" src:" << ofToString(thisSrc) << " layout:" << ofToString(thisLayout) << endl;
        thisFolder++;
      }


    }

    // MAPPINGS
    if(Settings::get().exists("mappings")){
      loadMidiMappings();
    }

    return sourceGroups;

  }

  void Config::loadMidiMappings(){
    // prefix
    string prefix = Settings::get().exists("mappings/") ? "mappings/" : "";

    // iterate midi keys
    for(auto &pair : midiMappingsStringsToCommand)
    {
        string k =  pair.first;//iter->first;
      int value = 0;
      if(Settings::get().exists(prefix+k)){
        value = Settings::getInt(prefix+k);
        midiMappings[k] = value;
        cout << k << ": " << value << endl;
      }

    }

      // midi config values: lowest note and ports
      if(Settings::get().exists(prefix+"first_midinote")){
          first_midinote = Settings::getInt(prefix+"first_midinote");
          cout << "first_midinote" << first_midinote << endl;
      }
      if(Settings::get().exists(prefix+"midi_ports")){
          std::vector<string> midi_portsStr = ofSplitString(Settings::getString(prefix+"midi_ports"),",");
          for(auto str : midi_portsStr){
              if(std::find(midi_ports.begin(), midi_ports.end(), stoi(str)) == midi_ports.end()) {
                  midi_ports.push_back(stoi(str));
              }
          }
      }

    // flop list for faster search on midi message
    for (map<string, int>::iterator i = midiMappings.begin(); i != midiMappings.end(); ++i){
      midiMapByValue[i->second] = midiMappingsStringsToCommand[i->first];
    }

  }

  void Config::loadDefaultMidiMappings(){
    if(ofFile::doesFileExist(ofToDataPath("../../midi_mappings.json"))){
        Settings::get().load(ofToDataPath("../../midi_mappings.json"));
        loadMidiMappings();
    }
  }
