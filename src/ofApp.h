#pragma once

#include <ctype.h>
#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxJsonSettings.h"
#include "ofxHapPlayer.h"

#define MAX_VIDEOS 88
#define N_LAYOUTS 5
#define MAX_LAYOUTPOS 4


class ofApp : public ofBaseApp, public ofxMidiListener{

	public:
		ofxHapPlayer  movie[MAX_VIDEOS];
		int folders[MAX_VIDEOS];
		int n_folders;
		int layout;

		int max_videos;
		int first_midinote;
        int midi_port;
		float fade_in;
		float fade_out;

		bool blending_multiply;

		void newMidiMessage(ofxMidiMessage& eventArgs);
		ofxMidiIn midiIn;
		void setupMidi();

		void setup();
		void update();
		void draw();

		void scanDataDir();
		void initVideoVariables(int key);
		void loadConfig(string path);
		void loadDefaultConfig();
		void loadDataDir();
		void loadFolders();
		void loadRandom();

		void drawVideoInLayout(int movieN);
		//void drawDiff(int movieN);


		void playVideo(int key, float vel);
		void deactivateVideo(int key);
		void stopVideo(int key);
		void changeAllSpeed(float control);
		float tapToSpeed(float t, int k);

		void stopSustain();
		void panic();

		void keyPressed(int key);
		void keyReleased(int key);


	private:

		// for multiply blending
		int thisLayoutInit[MAX_LAYOUTPOS];

		int screenW;
		int screenH;
		int n_videos;

		bool random;

		float speed; // global
		float sustain;

		bool active_videos[MAX_VIDEOS];

		bool sustained_videos[MAX_VIDEOS];
		float fo_start[MAX_VIDEOS];

		float tapTempo[MAX_VIDEOS];
		float tapSpeed[MAX_VIDEOS];
		float dyn[MAX_VIDEOS];

		float startPos[MAX_VIDEOS];

		// keep track of n_activeVideos for volume settings
		int n_activeVideos;
		float volume;

		// LAYOUTS
		// 1: dual vertical (0-1)
		// 2: dual horizontal (0-1)
		// 3: triple 2+1 (0-2)
        // 4: triptych (0-2)
		// 5: quad (0-3)

		int layoutCount[N_LAYOUTS+1][MAX_LAYOUTPOS]={0};

		int** layoutConf;


};
