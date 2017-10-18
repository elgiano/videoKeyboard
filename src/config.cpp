#include "config.h"

void Config::findConfig(){
  ofDirectory dir(ofToDataPath(""));
  // config file
  dir.allowExt("json");
  dir.listDir();
  dir.sort();

  loadDefaultConfig();
  if(dir.size()>0){
    cout << "custom conf" << endl;
    loadConfig(dir.getPath(0));
  }
}

  void Config::loadDefaultConfig(){
    cout << "default midi mappings" << endl;
    loadDefaultMidiMappings();
    cout << "default conf" << endl;
    loadConfig(ofToDataPath("../../defaultConf.json"));
  }

  void Config::loadConfig(string path){

    Settings::get().load(path);
    string enclosingDir = ofFilePath::getEnclosingDirectory(path);


    // LAYOUTS

    if(Settings::get().exists("layouts")){
      // count folders, init layoutConf
      n_layoutConfs = 0;
      int thisFolder = 0;
      while(Settings::get().exists("layouts/"+std::to_string(thisFolder++))){
        n_layoutConfs++;
      }

      layoutConf = new int*[n_layoutConfs];
      cout << ofToString(n_layoutConfs) <<" layout groups" << endl;

      // for each, split the data in N_LAYOUT numbers
      for(thisFolder = 0; thisFolder < n_layoutConfs; thisFolder++){
        std::vector<string> positions = ofSplitString(Settings::getString("layouts/"+std::to_string(thisFolder)),",");
        layoutConf[thisFolder] = new int[N_LAYOUTS];
        for(unsigned j=0;j<positions.size() && j<N_LAYOUTS;j++){
          layoutConf[thisFolder][j] = std::stoi(positions[j]);
        }
      }
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
        }

        if(Settings::get().exists("sources/"+std::to_string(thisFolder)+"/size")){
            thisRandSize = Settings::getInt("sources/"+std::to_string(thisFolder)+"/size");
        }

        // register source group
        SourceGroup thisSourceGroup{
          (short)(thisIsCapture?1:thisIsRandom?2:thisIsMultiple?3:0),
          thisLayout,
          thisDeviceID,
          thisRandSize,
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

  }

  void Config::loadMidiMappings(){
    // prefix
    string prefix = Settings::get().exists("mappings/") ? "mappings/" : "";

    // iterate midi keys
    for(std::map<string,int>::iterator iter = midiMappings.begin(); iter != midiMappings.end(); ++iter)
    {
      string k =  iter->first;
      int value = 0;
      if(Settings::get().exists(prefix+k)){
        value = Settings::getInt(prefix+k);
        midiMappings[k] = value;
        cout << k << ": " << value << endl;
      }
      if(Settings::get().exists(prefix+"first_midinote")){
        first_midinote = Settings::getInt(prefix+"first_midinote");
      }
      if(Settings::get().exists(prefix+"midi_port")){
        midi_port = Settings::getInt(prefix+"midi_port");
      }
      if(Settings::get().exists(prefix+"midi_port2")){
            midi_port2=Settings::getInt(prefix+"midi_port2");
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
