#include "ofApp.h"

// todo:
// * start/stop audio; OK (latency?)
// * stutter aftertouch
// * source shuffle mode


//--------------------------------------------------------------
void ofApp::setup(){

  ofSetLogLevel(OF_LOG_VERBOSE);
  ofLogToFile("log.txt");

  screenW = ofGetScreenWidth();
  screenH = ofGetScreenHeight();

  speed = 1.0;
  layout = 0;

  brightness = 0;
  brightness_opacity = 0;
  lastDecayTime = 0;


  n_captures = 0;
  n_videos = 0;


  for(int i=0;i<MAX_CAPTURE;i++){
    capture_keys[i] = -1;
  }

  //scanDataDir();
  //findConfig();
  settings.findConfig();
  first_midinote = settings.first_midinote;
  layoutConf = settings.layoutConf;
  loadSources();

  sustain_mode = 0;
  midiMaxVal = 127;
  setupMidi();

  ofBackground(0,0,0);
  //ofBackground(255,255,255);
  ofEnableAlphaBlending();
  ofSetFrameRate(120);
}

// ####### CAPTURE  #########

void ofApp::initCapture(){
    for(int i=0;i<n_captures; i++){
        capture[i].setDeviceID(capture_sources[i]);
        capture[i].setDesiredFrameRate(60);
        capture[i].initGrabber(screenW,screenH);
    }
}

ofVideoGrabber ofApp::captureFromKey(int key){

    int *found;

    found = std::find(capture_keys,capture_keys+MAX_CAPTURE,key);
    if(found != capture_keys+MAX_CAPTURE){
        key = std::distance(capture_keys,found);
        return capture[key];
    };
return capture[0];
}

bool ofApp::isCaptureKey (int key){

    return std::any_of(std::begin(capture_keys), std::end(capture_keys), [&](int i)
    {
        //cout << ofToString(key)<< endl;
        return ((i == key) && (i>=0));
    });
}

// ####### DATA DIR #########
/*
void ofApp::findConfig(){
  ofDirectory dir(ofToDataPath(""));
  // config file
  dir.allowExt("json");
  dir.listDir();
  dir.sort();

  loadDefaultConfig();
  if(dir.size()>0){
    cout << "custom conf" << endl;
    loadConfigNew(dir.getPath(0));
  }
}

void ofApp::loadConfigNew(string path){
  Settings::get().load(path);
  string enclosingDir = ofFilePath::getEnclosingDirectory(path);
  if( Settings::get().exists("general")){

      if(Settings::get().exists("general/fade_in")){
        fade_in=Settings::getFloat("general/fade_in");
      }
      if(Settings::get().exists("general/fade_out")){
        fade_out=Settings::getFloat("general/fade_out");
      }
      if(Settings::get().exists("general/blending_multiply")){
         blending_multiply=Settings::getBool("general/blending_multiply");
      }
      if(Settings::get().exists("general/random")){
         random=Settings::getBool("general/random");
      }
  }
  if(Settings::get().exists("layouts")){
    // count folders, init layoutConf
    n_layouts = 0;
    int thisFolder = 0;
    while(Settings::get().exists("layouts/"+std::to_string(thisFolder++))){
      n_layouts++;
    }

    layoutConf = new int*[n_layouts];
    cout << ofToString(n_layouts) <<" layout groups" << endl;

    // for each, split the data in N_LAYOUT numbers
    for(thisFolder = 0; thisFolder < n_layouts; thisFolder++){
      std::vector<string> positions = ofSplitString(Settings::getString("layouts/"+std::to_string(thisFolder)),",");
      layoutConf[thisFolder] = new int[N_LAYOUTS];
      for(unsigned j=0;j<positions.size() && j<N_LAYOUTS;j++){
        layoutConf[thisFolder][j] = std::stoi(positions[j]);
      }
    }

  }
  if(Settings::get().exists("sources")){
    int thisFolder = 0;
    while(Settings::get().exists("sources/"+std::to_string(thisFolder))){
      int thisLayout=0;
      int thisDeviceID=0;
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

      // load source group
      cout << "source group #"<< ofToString(thisFolder)<< " capture:" << ofToString(thisIsCapture) << " random:"<<ofToString(thisIsRandom) <<" src:" << ofToString(thisSrc) << " layout:" << ofToString(thisLayout) << endl;
      if(thisIsCapture){
        capture_sources[n_captures] = thisDeviceID;
        loadCaptureGroup(capture_sources[n_captures],thisLayout);
      }else{
        if(thisIsRandom){
          int randSize = 1;
          if(Settings::get().exists("sources/"+std::to_string(thisFolder)+"/size")){
              randSize = Settings::getInt("sources/"+std::to_string(thisFolder)+"/size");
          }
          loadRandomGroup(ofFilePath::join(enclosingDir,thisSrc),randSize);
        }else if(thisIsMultiple){
          loadMultipleGroup(thisSrc);
        }else{
          loadSourceGroup(ofFilePath::join(enclosingDir,thisSrc),thisLayout);
        }
      }

      thisFolder++;
    }
    if(n_captures>0){
      initCapture();
    }

  }
  if(Settings::get().exists("mappings")){
    loadMidiMappings();
  }

}

void ofApp::loadDefaultMidiMappings(){
    if(ofFile::doesFileExist(ofToDataPath("../../midi_mappings.json"))){
        Settings::get().load(ofToDataPath("../../midi_mappings.json"));
        loadMidiMappings();
    }
}

void ofApp::loadMidiMappings(){
  // iterate midi keys
  for(std::map<string,int>::iterator iter = midiMappings.begin(); iter != midiMappings.end(); ++iter)
  {
    string k =  iter->first;
    int value = 0;
    if(Settings::get().exists(k)){
      value = Settings::getInt(k);
      midiMappings[k] = value;
      cout << k << ": " << value << endl;
    }
    if(Settings::get().exists("first_midinote")){
      first_midinote = Settings::getInt("first_midinote");
    }
    if(Settings::get().exists("midi_port")){
      midi_port = Settings::getInt("midi_port");
    }
    if(Settings::get().exists("midi_port2")){
          midi_port2=Settings::getInt("midi_port2");
    }
  }

  // flop list for faster search on midi message
  for (map<string, int>::iterator i = midiMappings.begin(); i != midiMappings.end(); ++i){
    midiMapByValue[i->second] = midiMappingsStringsToCommand[i->first];
  }

}

void ofApp::loadDefaultConfig(){
  cout << "default midi mappings" << endl;
  loadDefaultMidiMappings();
  cout << "default conf" << endl;
  loadConfigNew(ofToDataPath("../../defaultConf.json"));
}*/
void ofApp::loadConfigNew(string path){}
void ofApp::loadSources(){
  n_captures = 0;
  cout << "loadSources() " << settings.n_sources << endl;
  for(int i =0;i<settings.n_sources;i++){
    SourceGroup src = settings.sourceGroups[i];
    /*cout << src.type << endl;
    cout << src.src << endl;
    cout << src.layout << endl;
    cout << src.deviceID << endl;
    cout << src.size << endl;*/

    switch(src.type){
      case 0: loadSourceGroup(src.src,src.layout);break;
      case 1: loadCaptureGroup(src.deviceID,src.layout);break;
      case 2: loadRandomGroup(src.src,src.size);break;
      case 3: loadMultipleGroup(src.src);break;
    }
  }
  if(n_captures>0){
    initCapture();
  }
}

// ## load videos ##

void ofApp::initVideoVariables(int key){
  active_videos[key] = false;
  tapTempo[key] = 0;
  tapSpeed[key] = 1.0;
  fo_start[key] = 0;
  fi_start[key] = 0;

  dyn[key] = 1;
  videoVolume[key] = 1;
  videoRms[key] = 1;
  startPos[key] = 0;
  stutterStart[key] = 0;
  stutterDur[key] = 0.1;
  movie[key].setLoopState(OF_LOOP_NORMAL);
  soundFader[key] = new SoundFader();
  soundFader[key]->setup(this,key);
}

std::map<string,float> ofApp::readRms(string path){
  std::map<string,float> volumes;
  std::ifstream infile(ofToDataPath(path+"/rms"));
  string a;
  float b;
  while (infile >> a >> b)
  {
     cout << path+"/"+a << " rms " << b <<endl;
     volumes[path+"/"+a] = b;
  }
  return volumes;

}

void ofApp::storeSetAvgRms(int set_n){
    float avgRms = 0;
    for(int i=setStart[set_n];i<n_videos;i++){
        avgRms += videoRms[i];
    }
    avgRms = avgRms/(n_videos-setStart[set_n]);
    setAvgRms[set_n] = avgRms;
    cout << "set " << set_n << " avgRms " << avgRms << endl;
}


void ofApp::loadMultipleGroup(string path){
  cout << "multiple group: " << path << endl;
  ofDirectory subdir(path);
  subdir.listDir();
  subdir.sort();
  loadedSets = 0;
  for(unsigned int i=0; i<subdir.size(); i++){
    ofDirectory thisPath(subdir.getPath(i));
    if(thisPath.isDirectory()){
      // load config
      thisPath.allowExt("json");
      thisPath.listDir();
      thisPath.sort();
      setStart[loadedSets++] = n_videos;
      if(thisPath.size()>0){
        loadConfigNew(thisPath.getPath(0));
      };
      storeSetAvgRms(loadedSets-1);

    }
  }
}

void ofApp::loadSourceGroup(string path,int layout){
    cout << "loading source group " << path << endl;
    ofDirectory subdir(path);
    subdir.allowExt("mov");
    subdir.listDir();
    subdir.sort();
    int tot_videos = subdir.size();
    std::map<string,float> volumes;
    // check rms file for clip volumes
    if(ofFile(path+"/rms").isFile()){
        volumes = readRms(path);
    }

    for(int k=0;k<tot_videos && n_videos<MAX_VIDEOS;k++){
      if(ofFile(subdir.getPath(k)).isFile()){
        movie[n_videos].load(subdir.getPath(k));
        initVideoVariables(n_videos);
        //cout << subdir.getPath(k) << endl;
        if(volumes[subdir.getPath(k)]>0){
          videoRms[n_videos] = volumes[subdir.getPath(k)];
        }
        layout_for_video[n_videos] = layout;
        n_videos++;

        cout << "Preloading " << n_videos  <<" "<< subdir.getPath(k)<< endl;

      }
    }
}

void ofApp::loadCaptureGroup(int deviceID,int layout){
      if(n_videos<MAX_VIDEOS){
        capture_sources[n_captures] = deviceID;
        capture_keys[n_captures] = n_videos;
        initVideoVariables(n_videos);
        layout_for_video[n_videos] = layout;

        cout << "Placing capture #"<< ofToString(n_captures) <<"at key " << ofToString(n_videos)  << endl;

        n_captures++;
        n_videos++;
      }
}

// random group
void ofApp::loadRandomGroup(string path,int size){
  ofDirectory dir(ofToDataPath(path));
  dir.allowExt("mov");
  dir.listDir();
  dir.sort();
  int n_sources = dir.size();

  for(int i=0;i<size && n_videos < MAX_VIDEOS;i++){
    string path = dir.getPath(round(ofRandom(0,n_sources-1)));
    movie[n_videos].load(path);
    initVideoVariables(i);
    layout_for_video[n_videos] = round(ofRandom(0,/*n_layouts*/settings.n_layoutConfs-1));
    startPos[n_videos] = ofRandom(0.0,1.0);
    n_videos++;
    cout << "Preloading random #" << i  <<": " << startPos[i] <<"@"<< path << endl;
  }

}

/*
void ofApp::loadRandom(){
  ofDirectory dir(ofToDataPath(""));
  dir.allowExt("mov");
  dir.listDir();
  dir.sort();
  n_videos = MAX_VIDEOS;
  int n_sources = dir.size();
  n_activeVideos = 0;

  for(int i=0;i<MAX_VIDEOS;i++){
    string path = dir.getPath(round(ofRandom(0,n_sources-1)));
    movie[i].load(path);
    initVideoVariables(i);
    layout_for_video[i] = round(ofRandom(0,n_layouts-1));
    startPos[i] = ofRandom(0.0,1.0);

    cout << "Preloading random #" << i  <<": " << startPos[i] <<"@"<< path << endl;
  }

}
*/

// ### DRAW ###

void ofApp::drawVideoInLayout(int movieN){
  float w = movie[movieN].getWidth();
  float h = movie[movieN].getHeight();

  float thisDyn = 1.0;
  if(isDynamic){
    thisDyn = dyn[movieN];
  }

  float fi_alpha = 1;
    // fade in
    if(fade_in>0 && isFading){
        fi_alpha = (movie[movieN].getPosition()-startPos[movieN])*movie[movieN].getDuration();
        fi_alpha = fi_alpha/fade_in;fi_alpha = fi_alpha <= 0 ? 0 : fi_alpha >= 1 ? 1 : fi_alpha;
    }


    // fade out
    //ofLogVerbose() << "fo " << ofToString(fo_start[movieN]);
    float fo_alpha = 1;
    //float fo_vol = 1;
    // if fade_out is 0 or fade switch is off, and video was already stopped, deactivate it
    // the real stopping of videos happens here for threading sake
    if(fo_start[movieN] > 0){
      if(fade_out==0 || !isFading){
          fo_alpha = 0;
          if((ofGetElapsedTimef()-fo_start[movieN])>sound_fadeTime){
              deactivateVideo(movieN);fo_start[movieN] = 0.0;return;
          }
      }else{
        // otherwise update the fade out
        fo_alpha = ofGetElapsedTimef()-fo_start[movieN];

        // kill video if fade ended
        if(fo_alpha>fade_out){deactivateVideo(movieN);fo_start[movieN] = 0.0;return;}

        fo_alpha = 1-(fo_alpha/fade_out);

      }
    }


  // read layout position
  int layoutPos=0;
  int thisLayout = layout_for_video[movieN];
    if(layoutShuffle){
        thisLayout = movieN % n_layouts;
    }
  if(layout>0 || layout < (-2)){
    layoutPos = layoutConf[thisLayout][abs(layout)-1];
  }else if(layout<0){
    layoutPos = abs(layoutConf[thisLayout][abs(layout)-1]-1);
  }

  //cout <<  ofToString(thisLayoutInit[0]) <<  ofToString(thisLayoutInit[1])<<  ofToString(thisLayoutInit[2])<< endl;
  // blending_multiply handling
  if(blending_multiply){
    // the background video is added, the rest are multiplied
    //if(thisLayoutInit[layoutPos]==0){
      //ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
      //cout << "FIRST LAYER" << ofToString(layoutPos) << endl;
      //thisLayoutInit[layoutPos]++ ;
  //  }else{

        /* draw brightness layer
        ofEnableAlphaBlending();
        ofSetColor(brightness, brightness, brightness, brightness_opacity);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());*/

      //cout << ofToString(thisLayoutInit[layoutPos])<< " pos"<< ofToString(layoutPos) << endl;
    //}
    ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);

    videoColor.set(255,255,255,255*fi_alpha*fo_alpha*thisDyn);
    //ofSetColor(255,255,255,255*fi_alpha*fo_alpha*thisDyn);
  }else if(blending_add){
    ofEnableBlendMode(OF_BLENDMODE_ADD);

    videoColor.set(255-brightness,255-brightness,255-brightness,255*fi_alpha*fo_alpha*thisDyn);
  }else{
    // alpha blending:
    ofEnableAlphaBlending();
    // logarithmic layering * fade_in_transparency * fade_out_transparency * dynamic level
    videoColor.set(255,255,255,255/log2(layoutCount[abs(layout)][layoutPos]+2)*fi_alpha*fo_alpha*thisDyn);
    //ofSetColor(255,255,255,255/log2(layoutCount[abs(layout)][layoutPos]+2)*fi_alpha*fo_alpha*thisDyn);
  }
  //videoColor.setSaturation(saturation);
  ofSetColor(videoColor);

  //ofLogVerbose() << "layout count " << ofToString(layoutCount[abs(layout)][layoutPos]) ;
  //ofLogVerbose() << "fi/fo " << ofToString(fi_alpha) << "/" <<  ofToString(fo_alpha);


    ofTexture thisTexture;
    if(isCaptureKey(movieN)){
        thisTexture.loadData(captureFromKey(movieN).getPixels());
        w = captureFromKey(movieN).getWidth();
        h = captureFromKey(movieN).getHeight();
    }else{
        thisTexture = *movie[movieN].getTexture();
        w = movie[movieN].getWidth();
        h = movie[movieN].getHeight();
    }

    //thisTexture = adjustBrightness(movie[movieN].getPixels(),w,h);

  // actual drawing in layout
  switch(abs(layout)){
    case 0:
          if( (blending_multiply || blending_add )&& thisLayoutInit[layoutPos]++==0){
              drawWhiteBg(0,(screenH-(screenW*h/w))/2, screenW, screenW*h/w);
          }
      thisTexture.draw(0,(screenH-(screenW*h/w))/2, screenW, screenW*h/w);
          if(blending_multiply){drawBrightnessLayer(0,(screenH-(screenW*h/w))/2, screenW, screenW*h/w);}

      break;
    case 1:
      // Split screen vertical
      //movie[movieN].draw(screenW/2*layoutPos,(screenH-(screenW/2*h/w))/2, screenW/2, screenW/2*h/w);
          if(blending_multiply && thisLayoutInit[layoutPos]++==0){
              drawWhiteBg(screenW/2*layoutPos,0,screenW/2,screenH);
          }
     thisTexture.drawSubsection(screenW/2*layoutPos,0,screenW/2,screenH,((screenH*w/h)-(screenW/2))*w/screenW/2,0,w*(1-((screenH*w/h)-(screenW/2))/screenW),h);
          if(blending_multiply){drawBrightnessLayer(screenW/2*layoutPos,0,screenW/2,screenH);}

      break;
    case 2:
      // Split screen horizontal
      //movie[movieN].draw((screenW-(screenH/2*w/h))/2,screenH/2*layoutPos, screenH/2*w/h, screenH/2)
          if(blending_multiply && thisLayoutInit[layoutPos]++==0){
              drawWhiteBg(0,screenH/2*layoutPos,screenW,screenH/2);
          }
      thisTexture.drawSubsection(0,screenH/2*layoutPos,screenW,screenH/2,0,((screenW*h/w)-(screenH/2))*h/screenH/2,w,h*(1-((screenW*h/w)-(screenH/2))/screenH));
      if(blending_multiply){drawBrightnessLayer(0,screenH/2*layoutPos,screenW,screenH/2);}

      break;
    case 3:
      // Split screen vertical and horizontal once
        if (layoutPos<2) {
            if(blending_multiply && thisLayoutInit[layoutPos]++==0){
                drawWhiteBg(screenW/2*layoutPos,(screenH/2-(screenW/2*h/w))/2+(layout>0?0:screenH/2), screenW/2, screenW/2*h/w);
            }
          thisTexture.draw(screenW/2*layoutPos,(screenH/2-(screenW/2*h/w))/2+(layout>0?0:screenH/2), screenW/2, screenW/2*h/w);
          if(blending_multiply){drawBrightnessLayer(screenW/2*layoutPos,(screenH/2-(screenW/2*h/w))/2+(layout>0?0:screenH/2), screenW/2, screenW/2*h/w);}
        }else{
            if(blending_multiply && thisLayoutInit[layoutPos]++==0){
                drawWhiteBg(0,(layout>0?screenH/2:0),screenW,screenH/2);
            }
          thisTexture.drawSubsection(0,(layout>0?screenH/2:0),screenW,screenH/2,0,((screenW*h/w)-(screenH/2))*h/screenH/2,w,h*(1-((screenW*h/w)-(screenH/2))/screenH));
          if(blending_multiply){drawBrightnessLayer(0,(layout>0?screenH/2:0),screenW,screenH/2);}

        };
      break;
    case 4:
    //triptych
          // x0,y0,w,h,
          // sx0,sy0, sw,sh
          if(blending_multiply && thisLayoutInit[layoutPos]++==0){
              drawWhiteBg(screenW/3*layoutPos,0,screenW/3,screenH);
          }
          thisTexture.drawSubsection(screenW/3*layoutPos,0,screenW/3,screenH,
                w/3,0,w/3,h
          );
          if(blending_multiply){drawBrightnessLayer(screenW/3*layoutPos,0,screenW/3,screenH);}

          break;
    case 5:
      // split in 4
          if(blending_multiply && thisLayoutInit[layoutPos]++==0){
              drawWhiteBg(screenW/2*(layoutPos%2),
                          (screenH/2*(layoutPos/2%2))+(screenH/2-(screenW/2*h/w))/2,
                          screenW/2, screenW/2*h/w);
          }
      thisTexture.draw(screenW/2*(layoutPos%2),
                        (screenH/2*(layoutPos/2%2))+(screenH/2-(screenW/2*h/w))/2,
                        screenW/2, screenW/2*h/w);
          if(blending_multiply){drawBrightnessLayer(screenW/2*(layoutPos%2),
                                                    (screenH/2*(layoutPos/2%2))+(screenH/2-(screenW/2*h/w))/2,
                                                    screenW/2, screenW/2*h/w);}


  }




}

void ofApp::drawBrightnessLayer(int x, int y, int w, int h){
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofSetColor(brightness, brightness, brightness);
    ofDrawRectangle(x, y, w, h);
}
void ofApp::drawWhiteBg(int x, int y, int w, int h){
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofSetColor(255, 255, 255);
    ofDrawRectangle(x, y+1, w, h-1);
    ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);

}

ofTexture ofApp::adjustBrightness(ofPixels pix,int w, int h){
    ofColor color;
    ofTexture newTexture;
    /*cout << pix.size() << endl;
    for(int r = 0; r<w; r++){
        for(int c = 0; c<h; c++){
            color = pix.getColor(r, c);
            //color.setBrightness(ofClamp(color.getBrightness()+100,0,255));
            //cout << color.getBrightness() << endl;
            pix.setColor(r,c,color);
        }
    }*/
    newTexture.loadData(pix);
    return newTexture;

}

//--------------------------------------------------------------
void ofApp::draw(){
  int* thisLayoutPos;

  for(int i=0; i<MAX_LAYOUTPOS; i++){thisLayoutInit[i]=0;}
  memset(layoutCount, 0, sizeof(layoutCount));
  // count videos in each layoutPos
  for(int i=0;i<MAX_VIDEOS;i++){if(active_videos[i] or sostenuto_videos[i] or sostenutoFreeze_videos[i]){
      int thisLayout = layout_for_video[i];
      if(layoutShuffle){
          thisLayout = i % n_layouts;
      }
    thisLayoutPos = layoutConf[thisLayout];
    layoutCount[0][0]++;
    for(int j=0;j<N_LAYOUTS;j++){
      layoutCount[j+1][thisLayoutPos[j]]++;
    }
  }};


  // draw videos
  for(int i=0;i<MAX_VIDEOS;i++){
    if(active_videos[i] or sostenuto_videos[i] or sostenutoFreeze_videos[i]){
      //if(movie[i].isFrameNew()){
      //ofLogVerbose() << "drawing " + ofToString(i);
      /*movie[i].setLoopState(loopState);
      cout << movie[i].getLoopState() << endl;*/
      drawVideoInLayout(i);
      //ofLogVerbose() << "drawn " + ofToString(i);
      //}
    }
  }
}

//--------------------------------------------------------------
void ofApp::update(){
  if(dynIsDecaying && ofGetElapsedTimef()-lastDecayTime>0.1){
    decayDyn();
    lastDecayTime = ofGetElapsedTimef();
  }
  for(int i=0;i<MAX_VIDEOS;i++){
    if(active_videos[i] or sostenuto_videos[i] or sostenutoFreeze_videos[i]){
        if(!movie[i].isPlaying()){
            movie[i].setSpeed(speed*(ribattutoSpeed?tapSpeed[i]:1));
            movie[i].setPosition(startPos[i]);
            movie[i].play();
        }else{
            if(isCaptureKey(i)){
                captureFromKey(i).update();
            }else{
              if(dynIsSpeed){
                movie[i].setSpeed(speed*dynToSpeed(i)*(ribattutoSpeed?tapSpeed[i]:1));
              }else{
                movie[i].setSpeed(speed*(ribattutoSpeed?tapSpeed[i]:1));
              }
              /*if(sostenutoFreeze_videos[i]){
                  movie[i].setPaused(true);
              }*/
                if(sostenutoFreeze_videos[i]){
                    if(
                       (movie[i].getPosition() > (stutterStart[i]+(stutterDur[i]*stutterDurGlobal/movie[i].getDuration())))
                       or (movie[i].getPosition() < stutterStart[i])
                       ){
                        cout << "stuttering " << i << endl;
                        movie[i].setPosition(stutterStart[i]);
                        movie[i].play();
                    }
                }
              if(reset_videos[i]){
                  // cout << "reset " << i << endl;
                  reset_videos[i] = false;
                  if(sostenutoFreeze_videos[i]){
                      movie[i].setPosition(stutterStart[i]);
                  }else{
                      movie[i].setPosition(startPos[i]);
                  }
                  setVideoVolume(i, 0);
                  movie[i].play();
                  continue;
               }

              //cout << dyn[i] << endl;

                soundFades(i);

                    if(harmonic_loops){
                      if(movie[i].getPosition()*movie[i].getDuration()>=harmonicLoopDur(i)){
                        movie[i].setPosition(startPos[i]);
                      }
                    }


              movie[i].update();
            }
            //ofLogVerbose() << "updated "+ofToString(i)+ofToString(active_videos[i]);
        }
    }else if(movie[i].isPlaying() ){movie[i].stop();}
  }
}

void ofApp::soundFades(int i){
  // sound fade in
    float now = ofGetElapsedTimef();
  float vol = 1;
  float vol_fo = 1;
  //if(((movie[i].getPosition()-startPos[i])*movie[i].getDuration())<sound_fadeTime){
  if((now-fi_start[i])<sound_fadeTime){

      //vol = (movie[i].getPosition()-startPos[i])*movie[i].getDuration();
      vol = (now - fi_start[i])/sound_fadeTime;
      //cout << "pct: " << vol << endl;
      vol = pow(vol,10);
      //cout << "pow: " << vol << endl;
      //vol = vol/sound_fadeTime;
  }
  // sound fade_out
  if(fo_start[i]>0){
      if(fade_out<=sound_fadeTime){
          vol_fo = 1-((now-fo_start[i])/sound_fadeTime);
      }else{
          vol_fo = 1-((now-fo_start[i])/fade_out);

      }
  }else{
     vol_fo = ((1-movie[i].getPosition())*movie[i].getDuration()/sound_fadeTime);
  }
    vol_fo = pow(vol_fo,10);

  vol = vol <= 0 ? 0 : vol >= 1 ? 1 : vol;
  vol_fo = vol_fo <= 0 ? 0 : vol_fo >= 1 ? 1 : vol_fo;

  if(vol<1 && vol_fo<1){
      vol = (vol_fo+vol)/2;
  }else{
      vol = vol_fo*vol;
  }
  //cout << "sf"<<i<<" vol:"<<vol<<endl;
  setVideoVolume(i,vol);

}

void ofApp::setVideoVolume(int key, float vol){
  if(rms_mode){
      float correction = setAvgRms[setNumberFromKey(key)]/videoRms[key];
      correction = 1+((correction-1)*rms_correction_pct);
    movie[key].setVolume(vol*volume*videoVolume[key]*correction);
      /*cout << "#"<<key<< " volume " << vol*volume*videoVolume[key]*setAvgRms[setNumberFromKey(key)]/videoRms[key] <<  " correction " << videoRms[key] << endl;
      cout << "set " << setNumberFromKey(key) << " avg: " << setAvgRms[setNumberFromKey(key)] << endl;*/
      //cout << "correction " << correction << endl;
  }else{
    movie[key].setVolume(vol*volume*videoVolume[key]);
  }
    //cout << "volume vol: " << vol <<endl;
     /*cout << "volume volume: " << volume <<endl;
     cout << "volume videoVol: " << videoVolume[key] <<endl;*/

}

int ofApp::setNumberFromKey(int key){
    for(int i=0;i<loadedSets;i++){
        if(setStart[i]>key){
            return ofClamp(i-1,0,loadedSets);
        }
    }
    return loadedSets>0?loadedSets-1:0;
}
// ### CONTROL ###

void ofApp::panic(){
  for(int i=0;i<MAX_VIDEOS;i++){
    //active_videos[i] = false;
    //movie[i].stop();
    deactivateVideo(i);
    //movie[i].setPosition(0);
  }
}

void ofApp::playVideo(int key, float vel){

  cout << "playing video KEY:" << key << " of set:" << activeSet << " setstart:" << setStart[activeSet] << endl;
  //ofLog(OF_LOG_VERBOSE,"starting video " + ofToString(key));
  if(key>=0 && key < MAX_VIDEOS){
    // update dynamics and stop fade_out
      dyn[key] = vel;
      videoVolume[key] = dynIsVolume ? vel : 1.0;
      setVideoVolume(key,1.0);
      fo_start[key] = 0;
      fi_start[key] = ofGetElapsedTimef();

    if(!active_videos[key]){
      active_videos[key] = true;
      reset_videos[key] = true;
      soundFader[key]->startThread();
      n_activeVideos++;
    }else if(sustain>0 && ribattutoSpeed){
      // if video is already active and sustain is on (tapping)
      float now = ofGetElapsedTimef();
      tapToSpeed(now-tapTempo[key],key);
      tapTempo[key] = now;
    }else{
      reset_videos[key] = true;
    }

    sustained_videos[key] = false;
    //sostenuto_videos[key] = false;
    //sostenutoFreeze_videos[key] = false;

  }
}

void ofApp::deactivateVideo(int key){
  fo_start[key] = 0.0;
  soundFader[key]->stopThread();
  active_videos[key] = false;

  //cout << "deactivating " << ofToString(key) << endl;

  // now the actual stop method is called by update()
  //movie[key].stop();

  n_activeVideos--;
}
void ofApp::stopVideo(int key){
  //key = key % n_videos;
  //key = setStart[activeSet] + key;

  ofLog(OF_LOG_VERBOSE, "stopping video " + ofToString(key));

  if(key>=0 && key < MAX_VIDEOS){
    //if(active_videos[key]){

      if(sustain==0 and !sostenuto_videos[key] and !sostenutoFreeze_videos[key]){
        // videos get deactivated by draw function when fade out is over
        fo_start[key] = ofGetElapsedTimef();
        // deactivateVideo(key);
        ofLog(OF_LOG_VERBOSE, "stopped video " + ofToString(key));

      }else{
          if(sustain>0){
              cout << "sustaining: "<< ofToString(key) << endl;

              sustained_videos[key] = true;
          }
          if(sostenuto_videos[key]>0 or sostenutoFreeze_videos[key]>0){
              active_videos[key] = false;
          }

      }
    //}
  }
}

// ## SPEED ##

void ofApp::changeAllSpeed(float control){
  float scaled =pow(3,ofMap(control,0,1,-2,2));
  /*  float scaled = 1.0;
    if(round(control*10)/10!=0.5){
        scaled =pow(3,ofMap(abs(control-0.5),0,0.5,-2,2))*-((control-0.5)>0?-1:1);
    }*/
  if(control>=0){
      speed = abs(scaled) * (speed_reverse?(-1):1);
  }else{
    // only update speed direction if control < 0
    speed = abs(speed) * (speed_reverse?(-1):1);
  }
  cout << "scaled: "<< ofToString(speed) << endl;
}

void ofApp::changeAllVolume(float control){
  float scaled =pow(10,ofMap(control,0.0,1.0,-10,0)/10);
    if(control==0){scaled=0;};

  volume = scaled;
  cout << control << " scaled: "<< ofToString(scaled) << endl;
}

float ofApp::dynToSpeed(int movieN){
  /*dyn[movieN] *= dynDecay;
  if(fo_start[movieN] > 0 && !sustained_videos[movieN] && !sostenuto_videos[movieN] && !sostenutoFreeze_videos[movieN]){
      dyn[movieN] *= dynDecay;
  }*/
  //cout <<dyn[movieN]<<endl;
  return pow(3,ofMap(dyn[movieN],0,1,-2,2));
}

void ofApp::decayDyn(){
  for(int i=0;i<MAX_VIDEOS;i++){
    dyn[i] *= dynDecay;
    if(fo_start[i] > 0 && !sustained_videos[i] && !sostenuto_videos[i] && !sostenutoFreeze_videos[i]){
        dyn[i] *= dynDecay;
    }
    dyn[i] = ofClamp(dyn[i],0.2,10);

  }
}

float ofApp::tapToSpeed(float t,int k){
  float newSpeed = 1.0;
  if(t>0 && t<= 10){
      newSpeed = pow(ofMap(t,0,10,1,0),10)*20;
  }
  tapSpeed[k] = newSpeed;
  cout << t << " " << ofToString(tapSpeed[k]) << endl;
  return tapSpeed[k];
};


// ## SUSTAINs ##

void ofApp::stopSustain(){
  for(int i=0;i<MAX_VIDEOS;i++){tapSpeed[i]=1.0;};
  cout << "stop sustain" << endl;

  for(int i=0;i<MAX_VIDEOS;i++){
    if(sustained_videos[i]){
      cout << "stop sustain " << i << endl;
      stopVideo(i);
      sustained_videos[i] = false;
    }
  }
}

void ofApp::startSostenuto(){
    sostenuto = 1;
    memcpy(sostenuto_videos,active_videos,MAX_VIDEOS);
}

void ofApp::stopSostenuto(){
    sostenuto = 0;
    for(int i=0;i<MAX_VIDEOS;i++){
        if(sostenuto_videos[i]){
            sostenuto_videos[i] = false;
            if(!active_videos[i]){
                stopVideo(i);
            }

        }
    };
}

void ofApp::startSostenutoFreeze(){
    sostenutoFreeze = 1;
    memcpy(sostenutoFreeze_videos,active_videos,MAX_VIDEOS);
    for(int i=0;i<MAX_VIDEOS;i++){
      if(sostenutoFreeze_videos[i]){
        stutterStart[i] = movie[i].getPosition();
        /*if(!stutterMode){
          stutterDur[i] = 0;
        }*/
      }
    }

}

void ofApp::stopSostenutoFreeze(){
    sostenutoFreeze = 0;
    for(int i=0;i<MAX_VIDEOS;i++){
        if(sostenutoFreeze_videos[i]){
            sostenutoFreeze_videos[i] = false;
            //movie[i].setPaused(false);
            if(!active_videos[i]){
                stopVideo(i);
            }

        }
    };
}

// ###

float ofApp::harmonicLoopDur(int key){
  int octave = floor((key - setStart[activeSet])/12);
  float ratio = semitoneToRatio[(first_midinote + key - setStart[activeSet]) % 12] ;
  float dur = harmonicLoopBaseDur / (ratio * octave);
    cout << dur << " = " << harmonicLoopBaseDur << " / (" << ratio << "x" << octave << ")" << endl;
  return harmonicLoopBaseDur / (ratio * octave);
}

// ### INPUT ####

void ofApp::setupMidi(){
  //ofSetVerticalSync(true);
  //ofSetLogLevel(OF_LOG_VERBOSE);

  cout << "setupMidi()" << endl;
  midiIn.openPort(settings.midi_port);
  midiIn.addListener(this);
    cout << settings.midi_port2 << endl;
  if(midi_port2>=0){
      midiIn2.openPort(settings.midi_port2);
      midiIn2.addListener(this);
  }
}

int ofApp::midiNoteToVideoKey(int note){
    int key;
    if(activeSet < (loadedSets-1)){
        key = setStart[activeSet] + (note%(setStart[activeSet+1]-setStart[activeSet]));
    }else{
        key = setStart[activeSet] + (note%(n_videos-setStart[activeSet]));
    }
    return key;
}

//--------------------------------------------------------------
void ofApp::newMidiMessage(ofxMidiMessage& msg) {

  cout << ofToString(msg.deltatime) << " ) " << msg.getStatusString(msg.status) << " " <<ofToString(((int)msg.pitch)) << " " << ofToString(msg.velocity) << " " << ofToString(msg.control) << " " << ofToString(msg.value)<< endl;

  switch(msg.status) {
    case MIDI_NOTE_ON :
      if(msg.velocity>0){
        playVideo(midiNoteToVideoKey(msg.pitch-first_midinote),(float) msg.velocity/127);
        break;
      }
    case MIDI_NOTE_OFF:
      stopVideo(midiNoteToVideoKey(msg.pitch-first_midinote));
      break;
    case MIDI_POLY_AFTERTOUCH:
      //cout << msg.value << endl;
      break;
    case MIDI_PITCH_BEND:
      msg.control = -1;
      midiMaxVal = 16383; // set maxVal to pitchBend
    case MIDI_CONTROL_CHANGE:
          //cout << ofToString(msg.control) << endl;
          switch(settings.midiMapByValue[msg.control]){
            case MidiCommand::fade_in:
              fade_in = (float)msg.value/midiMaxVal*3;
              cout << "fade_in:" << fade_in << endl;
              break;
      			case MidiCommand::fade_out:
              fade_out = (float)msg.value/midiMaxVal*3;
              cout << "fade_out:" << fade_out << endl;

              break;
      			case MidiCommand::global_speed:
              changeAllSpeed((float) msg.value/midiMaxVal);
              cout << "speed" << endl;
              break;
            case MidiCommand::speed_reverse:
              speed_reverse = msg.value > (midiMaxVal/2);
              changeAllSpeed(-1); // only update direction;
              cout << "speed reverse:" << speed_reverse << endl;
              break;
      			case MidiCommand::layout_change:
              layout = round(msg.value/(midiMaxVal/N_LAYOUTS/2))-N_LAYOUTS;
              cout << "layout " << ofToString(layout)<< endl;
              break;
      			case MidiCommand::brightness:
              brightness = (int)round((float)msg.value/midiMaxVal*255);
              cout << "brightness " << ofToString(brightness)<< endl;
              break;
              case MidiCommand::brightness_opacity:
                  brightness_opacity = (int)round((float)msg.value/midiMaxVal*255);
                  cout << "brightness transp: " << ofToString(brightness_opacity)<< endl;
                  break;
      			case MidiCommand::sustain:
              cout << "sustain" << endl;
              switch (sustain_mode) {
                case 0:
                  sustain = (msg.value)/midiMaxVal;
                  if(sustain==0){stopSustain();};
                  break;
                case 1:
                  cout << "sostenuto" << endl;
                  sostenuto = (msg.value)/midiMaxVal;
                  if(sostenuto==0){stopSostenuto();}
                  if(sostenuto==1){startSostenuto();}
                  break;
                case 2:
                  cout << "sostenutoFreeze" << endl;
                  sostenutoFreeze = (msg.value)/midiMaxVal;
                  if(sostenutoFreeze==0){stopSostenutoFreeze();}
                  if(sostenutoFreeze==1){startSostenutoFreeze();}
                  break;
              }
              break;
      			case MidiCommand::sostenuto:
              cout << "sostenuto" << endl;
              sostenuto = (msg.value)/midiMaxVal;
              if(sostenuto==0){stopSostenuto();}
              if(sostenuto==1){startSostenuto();}
              break;
      			case MidiCommand::sostenuto_freeze:
              sostenutoFreeze = (  msg.value)/midiMaxVal;
              if(sostenutoFreeze==0){stopSostenutoFreeze();}
              if(sostenutoFreeze==1){startSostenutoFreeze();}
              break;
      			case MidiCommand::dynamics_switch:
                  isDynamic = msg.value!=0;
              ofLogVerbose("dynamics_switch: " + ofToString(isDynamic));
              break;
      			case MidiCommand::fade_switch:
                  isFading = msg.value!=0 ;
                  ofLogVerbose("fading_switch: " + ofToString(isFading));
                break;
              case MidiCommand::blending_multiply_switch:
                           blending_multiply = msg.value!=0;
                           if(blending_multiply){blending_add=false;}

                      cout << "multiply " << blending_multiply<< endl;
              break;
              case MidiCommand::blending_add_switch:
                           blending_add = msg.value!=0;
                           if(blending_add){blending_multiply=false;}
                      cout << "add " << blending_add<< endl;
              break;
      			case MidiCommand::source_shuffle_switch:
              // TODO: not implemented
              break;
      	  	case MidiCommand::sustain_mode:
              sustain_mode = (int)round((float)msg.value/(midiMaxVal/2));
              break;
      	  	case MidiCommand::loop_mode:
              switch((int)round((float)msg.value/(midiMaxVal/2))){
                case 0:
                  cout << "loop none"<< endl;
                  loopState = OF_LOOP_NONE;
                  break;
                case 1:
                  cout << "loop normal" << endl;
                  loopState = OF_LOOP_NORMAL;
                  break;
                case 2:
                  cout << "loop rev" << endl;
                  loopState = OF_LOOP_PALINDROME;
                  break;
              }
              break;
              case MidiCommand::speed_dynamics:
                  cout << "dynIsSpeed" << endl;
                      dynIsSpeed = msg.value!=0;
                      cout << "dynIsSpeed " << dynIsSpeed << endl;
                      break;
              case MidiCommand::layout_shuffle:
                      layoutShuffle = msg.value!=0;
                      cout << "layout shuffle " <<layoutShuffle << endl;
                    break;
              case MidiCommand::dynamics_decay:
                    dynIsDecaying = msg.value!=0;
                    cout << "dynDecay " << dynIsDecaying << endl;

                  break;
              case MidiCommand::global_volume:
                  changeAllVolume((float)msg.value/midiMaxVal);
                  cout << "global_volume "<< volume << endl;
                  break;
              case MidiCommand::dynamics_volume:
                      dynIsVolume = msg.value!=0;
                      cout << "dynIsVolume "<< dynIsVolume << endl;
                    break;
              case MidiCommand::rms_normalize:
                    rms_mode = msg.value!=0;
                    cout << "rms_mode "<< rms_mode << endl;
                  break;
              case MidiCommand::stutter_mode:
                  //stutterMode = !stutterMode;
                  stutterDurGlobal = ofMap((float)msg.value/midiMaxVal,0,1,0.04,7);
                  //stutterMode = (stutterDurGlobal>0);
                  cout << "stutter_mode:" << stutterMode << endl;
                  cout << "sdg:" << stutterDurGlobal << endl;

                  break;
              case MidiCommand::stutter_dur_global:
                  stutterDurGlobal = (float)msg.value/midiMaxVal*0.5;
                  cout << "stutter_mode:" << stutterMode << endl;
                  break;
              case MidiCommand::harmonic_loops:
                  harmonic_loops = msg.value!=0;
                  cout << "harmonic_loops:" << harmonic_loops << endl;
                  break;
              case MidiCommand::harmonic_loop_base_dur:
                  harmonicLoopBaseDur = (float)msg.value/midiMaxVal*10;
                  cout << "harmonic_loops_baseDur:" << harmonicLoopBaseDur << endl;
                  break;
              case MidiCommand::switch_to_layout_0:
                  layout = 0;
                  cout << "layout " << ofToString(layout)<< endl;
                  break;
              case MidiCommand::switch_to_layout_1:
                  layout = 1;
                  cout << "layout " << ofToString(layout)<< endl;
                  break;
              case MidiCommand::switch_to_layout_2:
                  layout = 2;
                  cout << "layout " << ofToString(layout)<< endl;
                  break;
              case MidiCommand::switch_to_layout_3:
                  layout = 3;
                  cout << "layout " << ofToString(layout)<< endl;
                  break;
              case MidiCommand::switch_to_layout_4:
                  layout = 4;
                  cout << "layout " << ofToString(layout)<< endl;
                  break;
              case MidiCommand::switch_to_layout_5:
                  layout = 5;
                  cout << "layout " << ofToString(layout)<< endl;
                  break;
              case MidiCommand::switch_to_set_0:
                  activeSet = 0;
                  cout << "activeSet " << ofToString(activeSet)<< endl;
                  break;
              case MidiCommand::switch_to_set_1:
                  activeSet = 1%loadedSets;
                  cout << "activeSet " << ofToString(activeSet)<< endl;
                  break;
              case MidiCommand::switch_to_set_2:
                  activeSet = 2%loadedSets;
                  cout << "activeSet " << ofToString(activeSet)<< endl;
                  break;
              case MidiCommand::switch_to_set_3:
                  activeSet = 3%loadedSets;
                  cout << "activeSet " << ofToString(activeSet)<< endl;
                  break;
              case MidiCommand::switch_to_set_4:
                  activeSet = 4%loadedSets;
                  cout << "activeSet " << ofToString(activeSet)<< endl;
                  break;
              case MidiCommand::switch_to_set_5:
                  activeSet = 5%loadedSets;
                  cout << "activeSet " << ofToString(activeSet)<< endl;
                  break;
              case MidiCommand::ribattutoSpeed:
                  ribattutoSpeed = msg.value!=0;
                  cout << "ribattutoSpeed " << ofToString(ribattutoSpeed)<< endl;
                  break;
              case MidiCommand::panic:
                  panic();
                  cout << "panic " << endl;
                  break;
              case MidiCommand::sound_fade_time:
                  sound_fadeTime = pow((float)msg.value/midiMaxVal,10);
                  cout << "sound_fadeTime:" << sound_fadeTime << endl;
                  break;
              case MidiCommand::rms_correction_pct:
                  rms_correction_pct = (float)msg.value/midiMaxVal;
                  cout << "rms correction %:" << rms_correction_pct << endl;
                  break;
          }
      midiMaxVal = 127; // reset maxVal to control
      break;
    default:
      cout << ofToString(msg.deltatime) << " ) " << msg.getStatusString(msg.status) << " " <<ofToString(((int)msg.pitch)-21) << " " << ofToString(msg.control) << " " << ofToString(msg.value) << endl;
      break;
    };


}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

  key = tolower(key);
  if(key==(-1)){
    sustain = sustain == 0 ? 1 : 0;
    if(sustain == 0){stopSustain();}
  }else{
    playVideo(midiNoteToVideoKey(key-49),1.0);
  }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
  key = tolower(key);
  stopVideo(midiNoteToVideoKey(key-49));
  //cout << ofToString(key) << " released" << endl;
}
