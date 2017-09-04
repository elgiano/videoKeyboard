#include "ofApp.h"

// todo:
// * start/stop audio; OK (latency?)
// * try more videos (up to 88)
// * video speed (mode and scale)
// * midi input (connect dialog)

// multithread? control and playback!

//--------------------------------------------------------------
void ofApp::setup(){

  ofSetLogLevel(OF_LOG_VERBOSE);
  ofLogToFile("log.txt");

  screenW = ofGetScreenWidth();
  screenH = ofGetScreenHeight();

  speed = 1.0;
  layout = 0;

  saturation = 255;
  lastDecayTime = 0;


  n_captures = 0;
  n_videos = 0;

  for(int i=0;i<MAX_CAPTURE;i++){
    capture_keys[i] = -1;
  }

  //scanDataDir();
  findConfig();
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
    }
}

bool ofApp::isCaptureKey (int key){

    return std::any_of(std::begin(capture_keys), std::end(capture_keys), [&](int i)
    {
        //cout << ofToString(key)<< endl;
        return ((i == key) && (i>=0));
    });
}

// ####### DATA DIR #########

void ofApp::findConfig(){
  ofDirectory dir(ofToDataPath(""));
  // config file
  dir.allowExt("json");
  dir.listDir();
  dir.sort();

  loadDefaultConfig();
  if(dir.size()>0){
    loadConfigNew(dir.getPath(0));
  }
}

void ofApp::loadConfigNew(string path){
  Settings::get().load(path);
  if( Settings::get().exists("general")){
      cout << "custom conf" << endl;
      if(Settings::get().exists("general/first_midinote")){
        first_midinote=Settings::getInt("general/first_midinote");
      }
      if(Settings::get().exists("general/midi_port")){
          midi_port=Settings::getInt("general/midi_port");
      }
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
    n_layouts = 0; // TODO: change to n_layouts
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
      string thisSrc = "";
      //type, source, layout
      if(Settings::get().exists("sources/"+std::to_string(thisFolder)+"/type")){
          if(Settings::getString("sources/"+std::to_string(thisFolder)+"/type")=="capture"){
            thisIsCapture = true;
          }else if(Settings::getString("sources/"+std::to_string(thisFolder)+"/type")=="random"){
            thisIsRandom = true;
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
          loadRandomGroup(thisSrc,randSize);
        }else{
          loadSourceGroup(thisSrc,thisLayout);
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

void ofApp::loadMidiMappings(){
  // iterate midi keys
  for(std::map<string,int>::iterator iter = midiMappings.begin(); iter != midiMappings.end(); ++iter)
  {
    string k =  iter->first;
    int value = 0;
    if(Settings::get().exists("mappings/"+k)){
      value = Settings::getInt("mappings/"+k);
      midiMappings[k] = value;
      cout << k << ": " << value << endl;
    }
    if(Settings::get().exists("mappings/first_midinote")){
      first_midinote = Settings::getInt("mappings/first_midinote");
    }
    if(Settings::get().exists("mappings/midi_port")){
      midi_port = Settings::getInt("mappings/midi_port");
    }
  }

  // flop list for faster search on midi message
  for (map<string, int>::iterator i = midiMappings.begin(); i != midiMappings.end(); ++i){
    midiMapByValue[i->second] = midiMappingsStringsToCommand[i->first];
  }

}

void ofApp::loadDefaultConfig(){
  cout << "default conf" << endl;
  /*
  first_midinote = 21;
  midi_port = 1;
  fade_in = 0.01;
  fade_out = 0.1;
  blending_multiply = false;
  random = false;

  n_layouts = 4;

  layoutConf = new int*[n_layouts];

  layoutConf[0] = new int[N_LAYOUTS]{0,1,0,0,0};
  layoutConf[1] = new int[N_LAYOUTS]{1,0,1,1,1};
  layoutConf[2] = new int[N_LAYOUTS]{0,1,2,2,2};
  layoutConf[3] = new int[N_LAYOUTS]{1,0,3,1,3};

  n_captures = 0;
    for(int i =0; i< MAX_CAPTURE; i++){capture_keys[i] = -1;}*/
  loadConfigNew(ofToDataPath("../defaultConf.json"));
}

// ## load videos ##

void ofApp::initVideoVariables(int key){
  active_videos[key] = false;
  tapTempo[key] = 0;
  tapSpeed[key] = 1.0;
  fo_start[key] = 0;
  dyn[key] = 1;
  startPos[key] = 0;
  movie[key].setLoopState(OF_LOOP_NORMAL);
}

void ofApp::loadSourceGroup(string path,int layout){
    ofDirectory subdir(path);
    subdir.listDir();
    subdir.sort();
    int tot_videos = subdir.size();
    for(int k=0;k<tot_videos && n_videos<MAX_VIDEOS;k++){
      if(ofFile(subdir.getPath(k)).isFile()){
        movie[n_videos].load(subdir.getPath(k));
        initVideoVariables(n_videos);
        layout_for_video[n_videos] = layout;
        n_videos++;

        cout << "Preloading " << n_videos  <<" "<< subdir.getPath(k)<< endl;
      }
    }
}

void ofApp::loadCaptureGroup(int deviceID,int layout){
      if(n_videos<MAX_VIDEOS){
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
    layout_for_video[n_videos] = round(ofRandom(0,n_layouts-1));
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

/*void ofApp::setVolume(){
  volume = 1 - (n_activeVideos);
}*/

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
    // sound fade in
    /*float vol = (movie[movieN].getPosition()-startPos[movieN])*movie[movieN].getDuration();
    vol = vol/0.1;vol = vol <= 0 ? 0 : vol >= 1 ? 1 : vol;
    movie[movieN].setVolume(vol);
    cout << vol <<endl;*/

    // fade out
    //ofLogVerbose() << "fo " << ofToString(fo_start[movieN]);
    float fo_alpha = 1;
    // if fade_out is 0 or fade switch is off, and video was already stopped, deactivate it
    // the real stopping of videos happens here for threading sake
    if(fo_start[movieN] > 0){
      if(fade_out==0 || !isFading){
        deactivateVideo(movieN);fo_start[movieN] = 0.0;return;
      }else{
        // otherwise update the fade out
        fo_alpha = ofGetElapsedTimef()-fo_start[movieN];
        // kill video if fade ended
        if(fo_alpha>=fade_out){deactivateVideo(movieN);fo_start[movieN] = 0.0;return;}

        fo_alpha = 1-(fo_alpha/fade_out);
        fo_alpha = fo_alpha <= 0 ? 0 : fo_alpha >= 1 ? 1 : fo_alpha;
      }
    }
    /*if((fade_out==0 || !isFading) && fo_start[movieN] > 0){
        {deactivateVideo(movieN);fo_start[movieN] = 0.0;return;}
    }else if(fo_start[movieN] > 0 && fade_out>0 && isFading){
      // otherwise update the fade out
      fo_alpha = ofGetElapsedTimef()-fo_start[movieN];
      // kill video if fade ended
      if(fo_alpha>=fade_out){deactivateVideo(movieN);fo_start[movieN] = 0.0;return;}

      fo_alpha = 1-(fo_alpha/fade_out);
      fo_alpha = fo_alpha <= 0 ? 0 : fo_alpha >= 1 ? 1 : fo_alpha;

   }*/

  // read layout position
  int layoutPos=0;
  if(layout>0 || layout < (-2)){
    layoutPos = layoutConf[layout_for_video[movieN]][abs(layout)-1];
  }else if(layout<0){
    layoutPos = abs(layoutConf[layout_for_video[movieN]][abs(layout)-1]-1);
  }

  //cout <<  ofToString(thisLayoutInit[0]) <<  ofToString(thisLayoutInit[1])<<  ofToString(thisLayoutInit[2])<< endl;
  // blending_multiply handling
  if(blending_multiply){
    // the background video is added, the rest are multiplied
    if(thisLayoutInit[layoutPos]==0){
      ofEnableBlendMode(OF_BLENDMODE_ADD);
      //cout << "FIRST LAYER" << ofToString(layoutPos) << endl;
      thisLayoutInit[layoutPos] = thisLayoutInit[layoutPos] + 1 ;
    }else{
      ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
      //cout << ofToString(thisLayoutInit[layoutPos])<< " pos"<< ofToString(layoutPos) << endl;
    }
    videoColor.set(255,255,255,255*fi_alpha*fo_alpha*thisDyn);
    //ofSetColor(255,255,255,255*fi_alpha*fo_alpha*thisDyn);
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

    /*ofPixels pixels;
    thisTexture.readToPixels(pixels);
    for(auto & pixel:  pixels.getPixelsIter()){
      cout << ofToString(pixel.getColor()) << endl;
    }
    thisTexture.loadData(pixels,GL_RBGA);*/


  // actual drawing in layout
  switch(abs(layout)){
    case 0:
      thisTexture.draw(0,(screenH-(screenW*h/w))/2, screenW, screenW*h/w);
      break;
    case 1:
      // Split screen vertical
      //movie[movieN].draw(screenW/2*layoutPos,(screenH-(screenW/2*h/w))/2, screenW/2, screenW/2*h/w);
     thisTexture.drawSubsection(screenW/2*layoutPos,0,screenW/2,screenH,((screenH*w/h)-(screenW/2))*w/screenW/2,0,w*(1-((screenH*w/h)-(screenW/2))/screenW),h);
      break;
    case 2:
      // Split screen horizontal
      //movie[movieN].draw((screenW-(screenH/2*w/h))/2,screenH/2*layoutPos, screenH/2*w/h, screenH/2);
      thisTexture.drawSubsection(0,screenH/2*layoutPos,screenW,screenH/2,0,((screenW*h/w)-(screenH/2))*h/screenH/2,w,h*(1-((screenW*h/w)-(screenH/2))/screenH));
      break;
    case 3:
      // Split screen vertical and horizontal once
        if (layoutPos<2) {
          thisTexture.draw(screenW/2*layoutPos,(screenH/2-(screenW/2*h/w))/2+(layout>0?0:screenH/2), screenW/2, screenW/2*h/w);
        }else{
          thisTexture.drawSubsection(0,(layout>0?screenH/2:0),screenW,screenH/2,0,((screenW*h/w)-(screenH/2))*h/screenH/2,w,h*(1-((screenW*h/w)-(screenH/2))/screenH));
        };
      break;
    case 4:
    //triptych
          // x0,y0,w,h,
          // sx0,sy0, sw,sh
          thisTexture.drawSubsection(screenW/3*layoutPos,0,screenW/3,screenH,
                w/3,0,w/3,h
          );
          break;
    case 5:
      // split in 4
      thisTexture.draw(screenW/2*(layoutPos%2),
                        (screenH/2*(layoutPos/2%2))+(screenH/2-(screenW/2*h/w))/2,
                        screenW/2, screenW/2*h/w);


  }
}


//--------------------------------------------------------------
void ofApp::draw(){
  int* thisLayoutPos;

  for(int i=0; i<MAX_LAYOUTPOS; i++){thisLayoutInit[i]=0;}
  memset(layoutCount, 0, sizeof(layoutCount));
  // count videos in each layoutPos
  for(int i=0;i<MAX_VIDEOS;i++){if(active_videos[i]){
    thisLayoutPos = layoutConf[layout_for_video[i]];
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
  /*if(ofGetElapsedTimef()-lastDecayTime>0.1){
    decayDyn();
    lastDecayTime = ofGetElapsedTimef();
  }*/
  for(int i=0;i<MAX_VIDEOS;i++){
    if(active_videos[i] or sostenuto_videos[i] or sostenutoFreeze_videos[i]){
        if(!movie[i].isPlaying()){
            movie[i].setSpeed(speed*tapSpeed[i]);
            movie[i].setPosition(startPos[i]);
            movie[i].play();
        }else{
            if(isCaptureKey(i)){
                captureFromKey(i).update();
            }else{
              if(dynIsSpeed){
                movie[i].setSpeed(speed*tapSpeed[i]*dynToSpeed(i));
              }else{
                movie[i].setSpeed(speed*tapSpeed[i]);
              }
              if(sostenutoFreeze_videos[i]){
                  movie[i].setPaused(true);
              }
              if(reset_videos[i]){
                movie[i].setPosition(startPos[i]);
                reset_videos[i] = false;
              }
              //cout << dyn[i] << endl;
              if(movie[i].isFrameNew()){
              movie[i].update();}
            }
            //ofLogVerbose() << "updated "+ofToString(i)+ofToString(active_videos[i]);
        }
    }else if(movie[i].isPlaying() ){movie[i].stop();}
  }
}

// ### CONTROL ###

void ofApp::panic(){
  for(int i=0;i<MAX_VIDEOS;i++){
    active_videos[i] = false;
    movie[i].stop();
    movie[i].setPosition(0);
  }
}

void ofApp::playVideo(int key, float vel){
  key = key % n_videos;
  //ofLog(OF_LOG_VERBOSE,"starting video " + ofToString(key));
  if(key>=0 && key < MAX_VIDEOS){
    // update dynamics and stop fade_out
    dyn[key] = vel;
    fo_start[key] = 0;

    if(!active_videos[key]){
      active_videos[key] = true;
      reset_videos[key] = true;
      n_activeVideos++;
    }else if(sustain>0){
      // if video is already active and sustain is on (tapping)
      float now = ofGetElapsedTimef();
      tapToSpeed(now-tapTempo[key],key);
      tapTempo[key] = now;
    }else{
      reset_videos[key] = true;
    }

    sustained_videos[key] = false;
    sostenuto_videos[key] = false;
    sostenutoFreeze_videos[key] = false;

  }
}

void ofApp::deactivateVideo(int key){
  fo_start[key] = 0.0;
  active_videos[key] = false;
  //cout << "deactivating " << ofToString(key) << endl;

  // now the actual stop method is called by update()
  //movie[key].stop();

  n_activeVideos--;
}
void ofApp::stopVideo(int key){
  key = key % n_videos;
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
              sustained_videos[key] = true;
          }
          if(sostenuto>0){
              sostenuto_videos[key] = true;
              active_videos[key] = false;
          }
          if(sostenutoFreeze>0){
              sostenutoFreeze_videos[key] = true;
              active_videos[key] = false;
          }
      }
    //}
  }
}

void ofApp::changeAllSpeed(float control){
  float scaled =pow(3,2*control-1);
  speed = scaled;
  //cout << "scaled: "<< ofToString(scaled) << endl;
}

float ofApp::dynToSpeed(int movieN){
  /*dyn[movieN] *= dynDecay;
  if(fo_start[movieN] > 0 && !sustained_videos[movieN] && !sostenuto_videos[movieN] && !sostenutoFreeze_videos[movieN]){
      dyn[movieN] *= dynDecay;
  }*/
  //cout <<dyn[movieN]<<endl;
  return dyn[movieN];
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
    newSpeed = 1.0/t;
  }
  tapSpeed[k] = newSpeed;
  //cout << ofToString(tapSpeed[k]) << endl;
  return tapSpeed[k];
};

void ofApp::stopSustain(){
    for(int i=0;i<MAX_VIDEOS;i++){tapSpeed[i]=1.0;};
  for(int i=0;i<MAX_VIDEOS;i++){
    if(sustained_videos[i]){
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

// ### INPUT ####

void ofApp::setupMidi(){
  //ofSetVerticalSync(true);
  midiIn.openPort(midi_port);
  midiIn.addListener(this);
}

//--------------------------------------------------------------
void ofApp::newMidiMessage(ofxMidiMessage& msg) {

  //ofLogVerbose() << ofToString(msg.deltatime) << " ) " << msg.getStatusString(msg.status) << " " <<ofToString(((int)msg.pitch)) << " " << ofToString(msg.velocity) << " " << ofToString(msg.control) << " " << ofToString(msg.value);

  switch(msg.status) {
    case MIDI_NOTE_ON :
      if(msg.velocity>0){
        playVideo(msg.pitch-first_midinote,(float) msg.velocity/127);
        break;
      }
    case MIDI_NOTE_OFF:
      stopVideo(msg.pitch-first_midinote);
      break;
    case MIDI_PITCH_BEND:
      msg.control = -1;
      midiMaxVal = 16383; // set maxVal to pitchBend
    case MIDI_CONTROL_CHANGE:
          //cout << ofToString(msg.control) << endl;
          switch(midiMapByValue[msg.control]){
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
      			case MidiCommand::layout_change:
              layout = round(msg.value/(midiMaxVal/N_LAYOUTS/2))-N_LAYOUTS;
              cout << "layout " << ofToString(layout)<< endl;
              break;
      			case MidiCommand::saturation:
              saturation = (int)round((float)msg.value/midiMaxVal*255*2);
              cout << "saturation " << ofToString(saturation)<< endl;

              break;
      			case MidiCommand::sustain:
              cout << "sustain" << endl;
              switch (sustain_mode) {
                case 0:
                  sustain = (midiMaxVal-msg.value)/midiMaxVal;
                  if(sustain==0){stopSustain();};
                  break;
                case 1:
                  cout << "sostenuto" << endl;
                  sostenuto = (midiMaxVal-msg.value)/midiMaxVal;
                  if(sostenuto==0){stopSostenuto();}
                  if(sostenuto==1){startSostenuto();}
                  break;
                case 2:
                  cout << "sostenutoFreeze" << endl;
                  sostenutoFreeze = (midiMaxVal-msg.value)/midiMaxVal;
                  if(sostenutoFreeze==0){stopSostenutoFreeze();}
                  if(sostenutoFreeze==1){startSostenutoFreeze();}
                  break;
              }
              break;
      			case MidiCommand::sostenuto:
              cout << "sostenuto" << endl;
              sostenuto = (midiMaxVal-msg.value)/midiMaxVal;
              if(sostenuto==0){stopSostenuto();}
              if(sostenuto==1){startSostenuto();}
              break;
      			case MidiCommand::sostenuto_freeze:
              cout << "sostenutoFreeze" << endl;
              sostenutoFreeze = (midiMaxVal-msg.value)/midiMaxVal;
              if(sostenutoFreeze==0){stopSostenutoFreeze();}
              if(sostenutoFreeze==1){startSostenutoFreeze();}
              break;
      			case MidiCommand::dynamics_switch:
              isDynamic = !isDynamic;
              ofLogVerbose("dynamics_switch: " + ofToString(isDynamic));
              break;
      			case MidiCommand::fade_switch:
              isFading = !isFading;
              ofLogVerbose("fading_switch: " + ofToString(isFading));
              break;
      			case MidiCommand::blending_multiply_switch:
              cout << "multiply" << endl;
              blending_multiply = !blending_multiply;
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
    playVideo(key-49,1.0);
  }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
  key = tolower(key);
  stopVideo(key-49);
  //cout << ofToString(key) << " released" << endl;
}
