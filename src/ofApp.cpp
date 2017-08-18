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
    
  screenW = ofGetScreenWidth();
  screenH = ofGetScreenHeight();

  speed = 1.0;
  layout = 0;
    
  n_captures = 0;
    
  scanDataDir();
  //loadDataDir();
  //loadFolders();


  setupMidi();

  ofBackground(0,0,0);
  ofEnableAlphaBlending();
    


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
        return i == key && i>=0;
    });
}

// ####### DATA DIR #########

void ofApp::scanDataDir(){

  ofDirectory dir(ofToDataPath(""));
  // config file
  dir.allowExt("json");
  dir.listDir();
  dir.sort();

  if(dir.size()>0){
    loadConfig(dir.getPath(0));
  }else{
    loadDefaultConfig();
  }

  // subdirs
  //ofDirectory dir(ofToDataPath(""));
    dir.allowExt("");
  dir.listDir();
  dir.sort();

  if(random){
    loadRandom();
  }else{
    bool subdirs = false;
    for(unsigned i=0;i<dir.size();i++){
      if(dir.getFile(i).isDirectory()){subdirs=true;break;}
    }

    if(subdirs){
      loadFolders();
    }else{
      // no subfolders: load videos in data/
      loadDataDir();
    }
  }
}


void ofApp::loadConfig(string path){
  Settings::get().load(path);
  loadDefaultConfig();
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
  if(Settings::get().exists("folders")){
    // count folders, init layoutConf
    n_folders = 0;
    int thisFolder = 0;
    while(Settings::get().exists("folders/"+std::to_string(thisFolder++))){
      n_folders++;
    }

    layoutConf = new int*[n_folders];
    cout << ofToString(n_folders) <<" folders" << endl;

    // for each, split the data in N_LAYOUT numbers
    for(thisFolder = 0; thisFolder < n_folders; thisFolder++){
      std::vector<string> positions = ofSplitString(Settings::getString("folders/"+std::to_string(thisFolder)),",");
      layoutConf[thisFolder] = new int[N_LAYOUTS];
      for(unsigned j=0;j<positions.size() && j<N_LAYOUTS;j++){
        layoutConf[thisFolder][j] = std::stoi(positions[j]);
      }
    }

  }
    
    if(Settings::get().exists("capture")){
        // count folders, init layoutConf
        n_captures = 0;
        while(Settings::get().exists("capture/"+std::to_string(n_captures))){
            capture_sources[n_captures] = Settings::getInt("capture/"+ofToString(n_captures));
            if(Settings::get().exists("capture/layout/"+std::to_string(n_captures))){
                capture_layouts[n_captures] =  Settings::getInt("capture/layout/"+ofToString(n_captures));
            }
            
            cout << "capture #" << ofToString(n_captures) << " device:" << ofToString(capture_sources[n_captures]) << endl;
            
            n_captures++;
        }
        initCapture();
    }


}

void ofApp::loadDefaultConfig(){
  cout << "default conf" << endl;
  first_midinote = 21;
  midi_port = 1;
  fade_in = 0.01;
  fade_out = 0.1;
  blending_multiply = false;
  random = false;

  n_folders = 4;

  layoutConf = new int*[n_folders];

  layoutConf[0] = new int[N_LAYOUTS]{0,1,0,0,0};
  layoutConf[1] = new int[N_LAYOUTS]{1,0,1,1,1};
  layoutConf[2] = new int[N_LAYOUTS]{0,1,2,2,2};
  layoutConf[3] = new int[N_LAYOUTS]{1,0,3,1,3};
    
  n_captures = 0;
    for(int i =0; i< MAX_CAPTURE; i++){capture_keys[i] = -1;}
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

void ofApp::loadDataDir(){
  ofDirectory dir(ofToDataPath(""));
  dir.allowExt("mov");
  dir.listDir();
  dir.sort();
  n_videos = dir.size();
  n_activeVideos = 0;

  for(int i=0;i<MAX_VIDEOS && i<n_videos;i++){
    movie[i].load(dir.getPath(i));
    initVideoVariables(i);
    folders[i] = round(ofRandom(0,n_folders-1));

    cout << "Preloading " << i <<" "<< dir.getPath(i%n_videos) << endl;
  }
}

void ofApp::loadFolders(){
  ofDirectory dir(ofToDataPath(""));
  dir.listDir();
  dir.sort();
  unsigned n_dirs = 0;
  n_videos = 0;
  n_activeVideos = 0;

  for(unsigned i=0;i<dir.size();i++){
    if(!dir.getFile(i).isDirectory()){
      continue;
    }
    ofDirectory subdir(dir.getPath(i));
    subdir.listDir();
    subdir.sort();
    int tot_videos = subdir.size();
      if(tot_videos==0){
          // empty subfolder, assume capture
          for(int k=0; k<n_captures; k++){
              capture_keys[k] = n_videos;
              initVideoVariables(n_videos);
              folders[n_videos] = capture_layouts[k];
              
              cout << "Placing capture #"<< ofToString(k) <<"at key " << ofToString(n_videos)  << endl;
              
              n_videos++;
              
          }
      
      }else{
          // load all videos in subfolder
    for(int k=0;k<tot_videos;k++){
      movie[n_videos].load(subdir.getPath(k));
      initVideoVariables(n_videos);
      folders[n_videos] = n_dirs;

      n_videos++;

      cout << "Preloading " << n_videos  <<" "<< dir.getPath(i%n_videos) << endl;
    }
      }
    n_dirs++;
  }

}

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
    folders[i] = round(ofRandom(0,n_folders-1));
    startPos[i] = ofRandom(0.0,1.0);

    cout << "Preloading random #" << i  <<": " << startPos[i] <<"@"<< path << endl;
  }

}


/*void ofApp::setVolume(){
  volume = 1 - (n_activeVideos);
}*/

// ### DRAW ###

void ofApp::drawVideoInLayout(int movieN){
  float w = movie[movieN].getWidth();
  float h = movie[movieN].getHeight();
  float fi_alpha = 1;
    // fade in
    if(fade_in>0){
        fi_alpha = (movie[movieN].getPosition()-startPos[movieN])*movie[movieN].getDuration();
        fi_alpha = fi_alpha/fade_in;fi_alpha = fi_alpha <= 0 ? 0 : fi_alpha >= 1 ? 1 : fi_alpha;
    }
    // fade out
    //ofLogVerbose() << "fo " << ofToString(fo_start[movieN]);
    float fo_alpha = 1;
    if(fade_out==0 && fo_start[movieN] > 0){
        {deactivateVideo(movieN);fo_start[movieN] = 0.0;return;}
    }else if(fo_start[movieN] > 0 && fade_out>0){
      fo_alpha = ofGetElapsedTimef()-fo_start[movieN];
      
      if(fo_alpha>=fade_out){deactivateVideo(movieN);fo_start[movieN] = 0.0;return;}
   
      fo_alpha = 1-(fo_alpha/fade_out);
      fo_alpha = fo_alpha <= 0 ? 0 : fo_alpha >= 1 ? 1 : fo_alpha;
      
   }


  int layoutPos=0;
  if(layout>0 || layout < (-2)){
    layoutPos = layoutConf[folders[movieN]][abs(layout)-1];
  }else if(layout<0){
    layoutPos = abs(layoutConf[folders[movieN]][abs(layout)-1]-1);
  }

  //cout <<  ofToString(thisLayoutInit[0]) <<  ofToString(thisLayoutInit[1])<<  ofToString(thisLayoutInit[2])<< endl;

  // if blending_multiply

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
    ofSetColor(255,255,255,255*fi_alpha*fo_alpha*dyn[movieN]);
  }else{
    // alpha blending:
    // logarithmic layering * fade_in_transparency * fade_out_transparency * dynamic level
    ofSetColor(255,255,255,255/log2(layoutCount[abs(layout)][layoutPos]+2)*fi_alpha*fo_alpha*dyn[movieN]);
  }

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
    thisLayoutPos = layoutConf[folders[i]];
    layoutCount[0][0]++;
    for(int j=0;j<N_LAYOUTS;j++){
      layoutCount[j+1][thisLayoutPos[j]]++;
    }
  }};


  // draw videos
  for(int i=0;i<MAX_VIDEOS;i++){
    if(active_videos[i] or sostenuto_videos[i] or sostenutoFreeze_videos[i]){
      //ofLogVerbose() << "drawing " + ofToString(i);
      drawVideoInLayout(i);
      //ofLogVerbose() << "drawn " + ofToString(i);

    }
  }
}

//--------------------------------------------------------------
void ofApp::update(){
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
              movie[i].setSpeed(speed*tapSpeed[i]);
              if(sostenutoFreeze_videos[i]){
                  movie[i].setPaused(true);
                  cout << i <<" paused" <<endl;
              }
              movie[i].update();
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
          n_activeVideos++;
      }else if(sustain>0){
      // if video is already active and sustain is on (tapping)
      float now = ofGetElapsedTimef();
      tapToSpeed(now-tapTempo[key],key);
      tapTempo[key] = now;
    }
    sustained_videos[key] = false;
    sostenuto_videos[key] = false;
    sostenutoFreeze_videos[key] = false;

  }
}

void ofApp::deactivateVideo(int key){
  fo_start[key] = 0.0;
  active_videos[key] = false;
  cout << "deactivating " << ofToString(key) << endl;

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
    
    ofLogVerbose() << ofToString(msg.deltatime) << " ) " << msg.getStatusString(msg.status) << " " <<ofToString(((int)msg.pitch)) << " " << ofToString(msg.velocity) << " " << ofToString(msg.control) << " " << ofToString(msg.value);

	// make a copy of the latest message
	// midiMessage = msg;
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
      changeAllSpeed((float) msg.value/16383);
      break;
    case MIDI_CONTROL_CHANGE:
          switch(msg.control){
              case 1:
                  layout = round(msg.value/(127/N_LAYOUTS/2))-N_LAYOUTS;
                  break;
              case 63:
                  sustain = (127-msg.value)/127;
                  if(sustain==0){stopSustain();}
                  break;
              case 65:
                  sostenuto = (127-msg.value)/127;
                  if(sostenuto==0){stopSostenuto();}
                  if(sostenuto==1){startSostenuto();}
                  break;
              case 64:
                  sostenutoFreeze = (127-msg.value)/127;
                  if(sostenutoFreeze==0){stopSostenutoFreeze();}
                  if(sostenutoFreeze==1){startSostenutoFreeze();}
                  break;
          }
      
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
