#pragma once

#include <ctype.h>
#include <fstream>

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxJsonSettings.h"
#include "ofxHapPlayer.h"
#include "movieContainer.hpp"
#include "config.h"

#define MAX_VIDEOS 1056
#define MAX_SETS 12

#define MAX_CAPTURE 2
#define N_LAYOUTS 6 // fullscreen, double v, double h, triple, tryptich, quad
#define MAX_LAYOUTPOS 4

class SoundFader;

class ofApp : public ofBaseApp, public ofxMidiListener{

	public:
		Config settings;
		MovieContainer  movie[MAX_VIDEOS];
		int layout_for_video[MAX_VIDEOS];
		int n_layouts;
		int layout;

		int max_videos;
		int first_midinote;

		float fade_in;
		float fade_out;
		int sustain_mode;
		int brightness;
        int brightness_opacity;
		int black_screen=0;

		bool blending_multiply=true;
		bool blending_add=false;
		bool isDynamic = false;
		bool isFading = true;
		bool dynIsSpeed = false;
		bool dynIsVolume = false;
		bool dynIsDecaying = false;
    bool layoutShuffle = false;
		bool rms_mode = false;
		bool harmonic_loops = false;
		bool speed_reverse = false;

		bool stutterMode = true;

		float dynDecay = 0.98;
		float dynToSpeed(int movieN);
        float speedCurve = 3;
        float speedRange = 5;
		void decayDyn();
    
        bool mute = true;
    bool presentationMode = false;

		ofLoopType loopState = OF_LOOP_NORMAL;

		void setLoopState(ofLoopType type);
		
		ofColor videoColor;

        int midiNoteToVideoKey(int note);


		void newMidiMessage(ofxMidiMessage& eventArgs);
        std::vector<ofxMidiIn> midiInputs;
				std::vector<ofxMidiOut> midiOutputs;

		ofxMidiIn midiIn;
        ofxMidiIn midiIn2;

		void setupMidi();

        ofVideoGrabber capture[MAX_CAPTURE];
        int n_captures;
        int capture_sources[MAX_CAPTURE] = {0};
        int capture_keys[MAX_CAPTURE];
        int capture_layouts[MAX_CAPTURE];

        void initCapture();
        bool isCaptureKey( int key);
        ofVideoGrabber captureFromKey( int key );


		void setup();
		void update();
		void draw();
		void updateLayoutCount();


    void drawVideoInLayout(int movieN);
    void drawBrightnessLayer(int x, int y, int w, int h);
    void drawWhiteBg(int x, int y, int w, int h);

    ofTexture adjustBrightness(ofPixels pix,int w, int h);


		void findConfig();
		void scanDataDir();
		void initVideoVariables(int key);
		void loadConfig(string path);
		void loadDefaultConfig();
        void loadDefaultMidiMappings();
		/*void loadDataDir();
		void loadFolders();
		void loadRandom();*/

		void loadConfigNew(string path);
		void loadMidiMappings();
		void loadSources(std::vector<SourceGroup>);
		void registerAutoplayGroupForSet(int set_n,int first_video, int tot_videos);
		void loadSet(int set_n);
		void loadMultipleGroup(string path);
        int setNumberFromKey(int key);
		void loadSourceGroup(string path, int layout);
		void loadCaptureGroup(int deviceID, int layout);
		void loadRandomGroup(string path,int size);
		std::map<string,float> readRms(string path);
        void storeSetAvgRms(int set_n);
				void storeGlobalAvgRms();
		void setVideoVolume(int key,float vol);
		void soundFades(int i);

		void playVideo(int key, float vel);
		void deactivateVideo(int key);
		void stopVideo(int key);
		void changeAllSpeed(float control);
		void changeAllVolume(float control);
		float tapToSpeed(float t, int k);

		void stopSustain();
        void startSostenuto();
        void stopSostenuto();
        void startSostenutoFreeze();
        void stopSostenutoFreeze();
        void panic();

    float harmonicLoopDur(int key);

		void keyPressed(int key);
		void keyReleased(int key);


		float harmonicLoopBaseDur = 1.0;
		const float semitoneToRatio[12] = {
			1,1.0625,1.125,1.2,1.25,1.3333,1.375,1.5,1.625,1.6666,1.75,1.875
		};

    bool active_videos[MAX_VIDEOS];



	private:

		// for multiply blending:
		// register if layoutPos has been initialized (has already a texture)
		int thisLayoutInit[MAX_LAYOUTPOS];
    
    int layout_count_temp = 0 ;
    int layout_init_temp = 0 ;


		int screenW;
		int screenH;
		int n_videos;

		bool random;

		float speed; // global
		float sustain;
        float sostenuto;
        float sostenutoFreeze;
        bool sostenutoFreezeInhibit = false;
        bool sostenutoIsFreeze = false;

		bool reset_videos[MAX_VIDEOS] = {false};

		bool sustained_videos[MAX_VIDEOS];
    bool sostenuto_videos[MAX_VIDEOS];
    bool sostenutoFreeze_videos[MAX_VIDEOS];
    bool ribattutoSpeed = false;

		SoundFader* soundFader[MAX_VIDEOS];


		float fo_start[MAX_VIDEOS];
        float fi_start[MAX_VIDEOS];

        float sound_fadeTime = 0.01;

		float tapTempo[MAX_VIDEOS];
		float tapSpeed[MAX_VIDEOS];
		float dyn[MAX_VIDEOS];
		float videoVolume[MAX_VIDEOS];
		float videoRms[MAX_VIDEOS];
        float rms_correction_pct=1;
        bool rms_global = false;

		float startPos[MAX_VIDEOS];
		float stutterStart[MAX_VIDEOS];
		float stutterDur[MAX_VIDEOS];
    float stutterDurGlobal = 1;

		float lastDecayTime;
		// keep track of n_activeVideos for volume settings
		int n_activeVideos;
		float volume = 1.0;

		int activeSet = 0;
		int loadedSets = 0;
		int setStart[MAX_SETS] = {0};
    float setAvgRms[MAX_SETS] = {1};
		float globalAvgRms = 1;
		std::vector<std::array<int,2>> autoplayGroupsForSet[MAX_SETS];


		int midiMaxVal;
		// LAYOUTS
		// 1: dual vertical (0-1)
		// 2: dual horizontal (0-1)
		// 3: triple 2+1 (0-2)
    // 4: triptych (0-2)
		// 5: quad (0-3)

		int layoutCount[N_LAYOUTS+1][MAX_LAYOUTPOS]={0};

		//std::vector<int[N_LAYOUTS]> layoutConf;
};

class SoundFader : public ofThread {

public:
  int deltams = 1;

  void setup(ofApp *controller,int key) {
    movieN = key;
    ctrl = controller;
    ofAddListener(ofEvents().exit,this,&SoundFader::handleExit);
  }

  void threadedFunction() {
    ctrl->setVideoVolume(movieN,0);
    while(isThreadRunning()){
      ctrl->soundFades(movieN);
      std::this_thread::sleep_for(std::chrono::milliseconds(deltams));
    }
  }

  void handleExit(ofEventArgs &e)
  {
        if(isThreadRunning())
            waitForThread();
  }


private:
  ofApp *ctrl;
  int movieN;
};
