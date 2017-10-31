#include "ofApp.h"

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

  loadSources(settings.findConfig());
  first_midinote = settings.first_midinote;

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
    cout << "initCapture()" << endl;
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


// ## load videos ##

void ofApp::loadSources(std::vector<SourceGroup> sourceGroups){
  int old_captures = n_captures;
  cout << "loadSources() " << sourceGroups.size() << endl;
  for(auto src : sourceGroups){
    /*cout << src.type << endl;
    cout << src.src << endl;
    cout << src.layout << endl;
    cout << src.deviceID << endl;
    cout << src.size << endl;*/

    int old_videos = n_videos;

    switch(src.type){
      case 0: loadSourceGroup(src.src,src.layout);break;
      case 1: loadCaptureGroup(src.deviceID,src.layout);break;
      case 2: loadRandomGroup(src.src,src.size);break;
      case 3: loadMultipleGroup(src.src);break;
    }
    if(src.autoplay){
      registerAutoplayGroupForSet(max(0,loadedSets-1),old_videos-1,n_videos-old_videos);
    }
  }
  if(old_captures<n_captures){
    initCapture();
  }
}

void ofApp::registerAutoplayGroupForSet(int set_n,int first_video, int tot_videos){
  std::array<int,2> autoplay;autoplay[0] = first_video;autoplay[1] = tot_videos;
  autoplayGroupsForSet[set_n].push_back(autoplay);
  cout << "autoplay registered: set" << set_n << " from" << first_video << " for " << tot_videos << endl;
}

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
void ofApp::storeGlobalAvgRms(){
    float avgRms = 0;
    for(int i=0;i<loadedSets;i++){
        avgRms += pow(setAvgRms[i],2);
    }
    avgRms = pow(avgRms,-2)/(loadedSets);
    globalAvgRms = avgRms;
    cout << "global avgRms: " << avgRms << endl;
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
        //loadConfigNew(thisPath.getPath(0));
        cout << "load subset conf" << endl;
        loadSources(settings.loadConfig(thisPath.getPath(0)));
      };
      storeSetAvgRms(loadedSets-1);
    }
  }
  storeGlobalAvgRms();
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
        cout << "Preloading " << n_videos  <<" "<< subdir.getPath(k)<< endl;

        n_videos++;


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
    initVideoVariables(n_videos);
    layout_for_video[n_videos] = round(ofRandom(0,/*n_layouts*/settings.n_layoutConfs-1));
    startPos[n_videos] = ofRandom(0.0,1.0);
    cout << "Preloading random #" << n_videos  <<": " << startPos[n_videos] <<"@"<< path << endl;
    n_videos++;
  }

}

void ofApp::loadSet(int set_n){

  for(auto autoplay : autoplayGroupsForSet[activeSet]){
    cout << "autostop " << endl;
    for(int i=0;i<autoplay[1];i++){
      stopVideo(autoplay[0]+1);
    }
  }

  activeSet = set_n%loadedSets;

  for(auto autoplay : autoplayGroupsForSet[activeSet]){
    cout << "autoplay " << endl;
    for(int i=0;i<autoplay[1];i++){
      playVideo(autoplay[0]+1,1.0);
    }
  }
}

// ### DRAW ###

void ofApp::drawVideoInLayout(int movieN){
  float w = movie[movieN].getWidth();
  float h = movie[movieN].getHeight();

  float thisDyn = 1.0;
  float now = ofGetElapsedTimef();
  if(isDynamic){
    thisDyn = dyn[movieN];
  }

  float fi_alpha = 1;
    // fade in
    if(fade_in>0 && isFading){
        if(now-fi_start[movieN]<fade_in){
          fi_alpha = now-fi_start[movieN];
        }else{
          fi_alpha = (movie[movieN].getPosition()-startPos[movieN])*movie[movieN].getDuration();
        }
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
          if((now-fo_start[movieN])>sound_fadeTime){
              deactivateVideo(movieN);fo_start[movieN] = 0.0;return;
          }
      }else{
        // otherwise update the fade out
        fo_alpha = now-fo_start[movieN];

        // kill video if fade ended
        if(fo_alpha>fade_out){deactivateVideo(movieN);fo_start[movieN] = 0.0;return;}

        fo_alpha = 1-(fo_alpha/fade_out);

      }
    }


  // read layout position
  int layoutPos=0;
  int thisLayout = layout_for_video[movieN];
    if(layoutShuffle){
        thisLayout = movieN % settings.n_layoutConfs;
    }
  if(layout>0 || layout < (-2)){
    layoutPos = settings.layoutConf[thisLayout][abs(layout)-1];
  }else if(layout<0){
    layoutPos = abs(settings.layoutConf[thisLayout][abs(layout)-1]-1);
  }

  if(blending_multiply){

    ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
    videoColor.set(255,255,255,255*fi_alpha*fo_alpha*thisDyn*(1.0-((float)brightness/255.0)));

  }else if(blending_add){

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    videoColor.set(255-brightness,255-brightness,255-brightness,255*fi_alpha*fo_alpha*thisDyn*(1.0-((float)brightness/255.0)));

  }else{
    // alpha blending:
    ofEnableAlphaBlending();
    // logarithmic layering * fade_in_transparency * fade_out_transparency * dynamic level
    videoColor.set(255,255,255,255/log2(layoutCount[abs(layout)][layoutPos]+2)*fi_alpha*fo_alpha*thisDyn);
  }
  //videoColor.setSaturation(saturation);
  ofSetColor(videoColor);


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
          //if(blending_multiply){drawBrightnessLayer(0,(screenH-(screenW*h/w))/2, screenW, screenW*h/w);}

      break;
    case 1:
      // Split screen vertical
      //movie[movieN].draw(screenW/2*layoutPos,(screenH-(screenW/2*h/w))/2, screenW/2, screenW/2*h/w);
          if(blending_multiply && thisLayoutInit[layoutPos]++==0){
              drawWhiteBg(screenW/2*layoutPos,0,screenW/2,screenH);
          }
     thisTexture.drawSubsection(screenW/2*layoutPos,0,screenW/2,screenH,((screenH*w/h)-(screenW/2))*w/screenW/2,0,w*(1-((screenH*w/h)-(screenW/2))/screenW),h);
         // if(blending_multiply){drawBrightnessLayer(screenW/2*layoutPos,0,screenW/2,screenH);}

      break;
    case 2:
      // Split screen horizontal
      //movie[movieN].draw((screenW-(screenH/2*w/h))/2,screenH/2*layoutPos, screenH/2*w/h, screenH/2)
          if(blending_multiply && thisLayoutInit[layoutPos]++==0){
              drawWhiteBg(0,screenH/2*layoutPos,screenW,screenH/2);
          }
      thisTexture.drawSubsection(0,screenH/2*layoutPos,screenW,screenH/2,0,((screenW*h/w)-(screenH/2))*h/screenH/2,w,h*(1-((screenW*h/w)-(screenH/2))/screenH));
     // if(blending_multiply){drawBrightnessLayer(0,screenH/2*layoutPos,screenW,screenH/2);}

      break;
    case 3:
      // Split screen vertical and horizontal once
        if (layoutPos<2) {
            if(blending_multiply && thisLayoutInit[layoutPos]++==0){
                drawWhiteBg(screenW/2*layoutPos,(screenH/2-(screenW/2*h/w))/2+(layout>0?0:screenH/2), screenW/2, screenW/2*h/w);
            }
          thisTexture.drawSubsection(screenW/2*layoutPos,	(layout>0?0:screenH/2), screenW/2, screenH/2,
              w*((1-(screenW/screenH))/2),0,
              w*(screenW/screenH),h);
          if(blending_multiply){drawBrightnessLayer(screenW/2*layoutPos,(screenH/2-(screenW/2*h/w))/2+(layout>0?0:screenH/2), screenW/2, screenW/2*h/w);}
        }else{
            if(blending_multiply && thisLayoutInit[layoutPos]++==0){
                drawWhiteBg(0,(layout>0?screenH/2:0),screenW,screenH/2);
            }
          thisTexture.drawSubsection(0,(layout>0?screenH/2:0),screenW,screenH/2,0,((screenW*h/w)-(screenH/2))*h/screenH/2,w,h*(1-((screenW*h/w)-(screenH/2))/screenH));
         // if(blending_multiply){drawBrightnessLayer(0,(layout>0?screenH/2:0),screenW,screenH/2);}

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
         // if(blending_multiply){drawBrightnessLayer(screenW/3*layoutPos,0,screenW/3,screenH);}

          break;
    case 5:
      // split in 4
          if(blending_multiply && thisLayoutInit[layoutPos]++==0){
              drawWhiteBg(screenW/2*(layoutPos%2),(screenH/2*(layoutPos/2%2))+(screenH/2-(screenW/2*h/w))/2,
                          screenW/2, screenH/2);

          }
          thisTexture.drawSubsection(screenW/2*(layoutPos%2),
                           (screenH/2*(layoutPos/2%2))+(screenH/2-(screenW/2*h/w))/2,
                           screenW/2, screenH/2,
                           w*((1-(screenW/screenH))/2),0,
                           w*(screenW/screenH),h);


          /*if(blending_multiply){drawBrightnessLayer(screenW/2*(layoutPos%2),
                                                    (screenH/2*(layoutPos/2%2))+(screenH/2-(screenW/2*h/w))/2,
                                                    screenW/2, screenH/2);
}*/


  }


}

void ofApp::drawBrightnessLayer(int x, int y, int w, int h){
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofSetColor(brightness, brightness, brightness);
    ofDrawRectangle(x, y+1, w, h-1);
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

  updateLayoutCount();

  // draw videos
  for(int i=0;i<MAX_VIDEOS;i++){
    if(active_videos[i] or sostenuto_videos[i] or sostenutoFreeze_videos[i]){
      /*movie[i].setLoopState(loopState);
      cout << movie[i].getLoopState() << endl;*/
      drawVideoInLayout(i);

    }
  }
  if(black_screen>0){
    ofSetColor(0,0,0,black_screen);
    ofEnableAlphaBlending();
    ofDrawRectangle(0,0,screenW,screenH);
  }
}

void ofApp::updateLayoutCount(){
  // count videos in each layoutPos
  memset(thisLayoutInit,0,sizeof(thisLayoutInit));
  memset(layoutCount, 0, sizeof(layoutCount));
  for(int i=0;i<MAX_VIDEOS;i++){if(active_videos[i] or sostenuto_videos[i] or sostenutoFreeze_videos[i]){
    int thisLayout = layoutShuffle?(i % settings.n_layoutConfs):layout_for_video[i];
    layoutCount[0][0]++;
    for(int j=0;j<(N_LAYOUTS-1);j++){
      layoutCount[j+1][settings.layoutConf[thisLayout][j]]++;
    }
  }};
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
                  //setVideoVolume(i, 0);
                  movie[i].play();
                  continue;
               }

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
  }else if(movie[i].getPosition()*movie[i].getDuration()<sound_fadeTime){
    vol = movie[i].getPosition()*movie[i].getDuration()/sound_fadeTime;
    //cout << "pct: " << vol << endl;
    vol = pow(vol,10);
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
      correction = globalAvgRms/videoRms[key];
      correction = 1+((correction-1)*rms_correction_pct);
    movie[key].setVolume(vol*volume*videoVolume[key]*correction);
      /*cout << "#"<<key<< " volume " << vol*volume*videoVolume[key]*setAvgRms[setNumberFromKey(key)]/videoRms[key] <<  " correction " << videoRms[key] << endl;
      cout << "set " << setNumberFromKey(key) << " avg: " << setAvgRms[setNumberFromKey(key)] << endl;*/
      //cout << "correction " << correction << endl;
  }else{
    movie[key].setVolume(vol*volume*videoVolume[key]);
  }

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
      //setVideoVolume(key,1.0);
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
  //cout << "stop sustain" << endl;

  for(int i=0;i<MAX_VIDEOS;i++){
    if(sustained_videos[i]){
      //cout << "stop sustain " << i << endl;
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
  float ratio = semitoneToRatio[(settings.first_midinote + key - setStart[activeSet]) % 12] ;
  float dur = harmonicLoopBaseDur / (ratio * octave);
    cout << dur << " = " << harmonicLoopBaseDur << " / (" << ratio << "x" << octave << ")" << endl;
  return harmonicLoopBaseDur / (ratio * octave);
}

// ### INPUT: midi input and setup is in midiInterface.cpp ####

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
