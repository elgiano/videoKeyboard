#include "ofApp.h"

// todo:
// * start/stop audio; OK (latency?)
// * try more videos (up to 88)
// * video speed (mode and scale)
// * midi input (connect dialog)

// multithread? control and playback!

//--------------------------------------------------------------
void ofApp::setup(){
  screenW = ofGetScreenWidth();
  screenH = ofGetScreenHeight();

  speed = 1.0;
  layout = 0;

  scanDataDir();
  //loadDataDir();
  //loadFolders();


  setupMidi();

  ofBackground(0,0,0);
  ofEnableAlphaBlending();


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
      if(Settings::get().exists("general/fade_in")){
        fade_in=Settings::getInt("general/fade_in");
      }
      if(Settings::get().exists("general/fade_out")){
        fade_out=Settings::getInt("general/fade_out");
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

}

void ofApp::loadDefaultConfig(){
  cout << "default conf" << endl;
  first_midinote = 21;
  fade_in = 0.01;
  fade_out = 0.1;
  blending_multiply = false;
  random = false;

  n_folders = 4;

  layoutConf = new int*[n_folders];

  layoutConf[0] = new int[n_folders]{0,1,0,0};
  layoutConf[1] = new int[n_folders]{1,0,1,1};
  layoutConf[2] = new int[n_folders]{0,1,2,2};
  layoutConf[3] = new int[n_folders]{1,0,3,3};
}

// ## load videos ##

void ofApp::initVideoVariables(int key){
  active_videos[key] = false;
  tapTempo[key] = 0;
  tapSpeed[key] = 1.0;
  fo_start[key] = 0;
  dyn[key] = 1;
  startPos[key] = 0;
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
    for(int k=0;k<tot_videos;k++){
      movie[n_videos].load(subdir.getPath(k));
      initVideoVariables(n_videos);
      folders[n_videos] = n_dirs;

      n_videos++;

      cout << "Preloading " << n_videos  <<" "<< dir.getPath(i%n_videos) << endl;
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
  // fade in
  float p = movie[movieN].getPosition()*movie[movieN].getDuration();
  if(fade_in>0){p = p/fade_in;p = p <= 0 ? 0 : p >= 1 ? 1 : p;}else{p=1;}
  // fade out
  float fo_trans = 1;
  if(fo_start[movieN] > 0){
    fo_trans =ofGetElapsedTimef()-fo_start[movieN];
    if(fo_trans>fade_out){deactivateVideo(movieN);fo_trans = 0;}
    else{
      if(fade_out>0){
        fo_trans = 1-(fo_trans/fade_out);
        fo_trans = fo_trans <= 0 ? 0 : fo_trans >= 1 ? 1 : fo_trans;
      }
    }
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
    //ofSetColor(255,255,255,255/p*fo_trans);
  }else{
    // alpha blending:
    // logarithmic layering * fade_in_transparency * fade_out_transparency * dynamic level
    ofSetColor(255,255,255,255/log2(layoutCount[abs(layout)][layoutPos]+2)*p*fo_trans);
  }


  switch(abs(layout)){
    case 0:
      movie[movieN].draw(0,(screenH-(screenW*h/w))/2, screenW, screenW*h/w);
      break;
    case 1:
      // Split screen vertical
      //movie[movieN].draw(screenW/2*layoutPos,(screenH-(screenW/2*h/w))/2, screenW/2, screenW/2*h/w);
      movie[movieN].getTexture()->drawSubsection(screenW/2*layoutPos,0,screenW/2,screenH,((screenH*w/h)-(screenW/2))*w/screenW/2,0,w*(1-((screenH*w/h)-(screenW/2))/screenW),h);
      break;
    case 2:
      // Split screen horizontal
      //movie[movieN].draw((screenW-(screenH/2*w/h))/2,screenH/2*layoutPos, screenH/2*w/h, screenH/2);
      movie[movieN].getTexture()->drawSubsection(0,screenH/2*layoutPos,screenW,screenH/2,0,((screenW*h/w)-(screenH/2))*h/screenH/2,w,h*(1-((screenW*h/w)-(screenH/2))/screenH));
      break;
    case 3:
      // Split screen vertical and horizontal once
        if (layoutPos<2) {
          movie[movieN].draw(screenW/2*layoutPos,(screenH/2-(screenW/2*h/w))/2+(layout>0?0:screenH/2), screenW/2, screenW/2*h/w);
        }else{
          movie[movieN].getTexture()->drawSubsection(0,(layout>0?screenH/2:0),screenW,screenH/2,0,((screenW*h/w)-(screenH/2))*h/screenH/2,w,h*(1-((screenW*h/w)-(screenH/2))/screenH));
        };
      break;
    case 4:
      // split in 4
      movie[movieN].draw(screenW/2*(layoutPos%2),
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
    if(/*movie[i].isPlaying()*/active_videos[i]){
      drawVideoInLayout(i);
    }
  }
}

//--------------------------------------------------------------
void ofApp::update(){
  for(int i=0;i<MAX_VIDEOS;i++){
    if(active_videos[i]){
      movie[i].update();
    }
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
  if(key>=0 && key < MAX_VIDEOS){
    if(active_videos[key]==false){
      movie[key].setSpeed(speed);
      movie[key].setPosition(startPos[key]);
      movie[key].play();
      active_videos[key] = true;
      dyn[key] = vel;
      n_activeVideos++;
    }else if(active_videos[key] && sustain>0){
      // if video is already active and sustain is on (tapping)
      float now = ofGetElapsedTimef();
      movie[key].setSpeed(speed*tapToSpeed(now-tapTempo[key],key));
      tapTempo[key] = now;
    }
    sustained_videos[key] = false;
  }
}

void ofApp::deactivateVideo(int key){
  active_videos[key] = false;
  fo_start[key] = 0;
  movie[key].stop();
  n_activeVideos--;
}
void ofApp::stopVideo(int key){
  key = key % n_videos;
  if(key>=0 && key < MAX_VIDEOS){
    if(active_videos[key]){
      if(sustain==0){
        fo_start[key] = ofGetElapsedTimef();
        //deactivateVideo(key);
      }else{
        sustained_videos[key] = true;
      }
    }
  }
}

void ofApp::changeAllSpeed(float control){
  float scaled =pow(3,2*control-1);
  speed = scaled;
  for(int i=0;i<MAX_VIDEOS;i++){
    if(active_videos[i]){
      movie[i].setSpeed(scaled*tapSpeed[i]);
    };
  };
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
  for(int i=0;i<MAX_VIDEOS;i++){
    if(sustained_videos[i]){
      stopVideo(i);
      sustained_videos[i] = false;
    }
  }
}

// ### INPUT ####

void ofApp::setupMidi(){
  ofSetVerticalSync(true);
  midiIn.openPort(1);
  midiIn.addListener(this);
}

//--------------------------------------------------------------
void ofApp::newMidiMessage(ofxMidiMessage& msg) {

	// make a copy of the latest message
	// midiMessage = msg;
  switch(msg.status) {
    case MIDI_NOTE_ON :
      if(msg.velocity>0){
        playVideo(msg.pitch-first_midinote,msg.velocity);
        break;
      }
    case MIDI_NOTE_OFF:
      stopVideo(msg.pitch-first_midinote);
      break;
    case MIDI_PITCH_BEND:
      changeAllSpeed((float) msg.value/16383);
      break;
    case MIDI_CONTROL_CHANGE:
      layout = round(msg.value/(127/8))-4;
      break;
    default:
      cout << ofToString(msg.deltatime) << " ) " << msg.getStatusString(msg.status) << " " <<ofToString(((int)msg.pitch)-21) << " " << ofToString(msg.control) << " " << ofToString(msg.value) << endl;
      break;
    };

	cout << ofToString(msg.deltatime) << " ) " << msg.getStatusString(msg.status) << " " <<ofToString(((int)msg.pitch)-21) << " " << ofToString(msg.control) << " " << ofToString(msg.value) << endl;
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
