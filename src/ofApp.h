#pragma once

#include <ctype.h>
#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxJsonSettings.h"
#include "ofxHapPlayer.h"

#define MAX_VIDEOS 88
#define MAX_CAPTURE 2
#define N_LAYOUTS 6 // fullscreen, double v, double h, triple, tryptich, quad
#define MAX_LAYOUTPOS 4


class ofApp : public ofBaseApp, public ofxMidiListener{

	public:
		ofxHapPlayer  movie[MAX_VIDEOS];
		int layout_for_video[MAX_VIDEOS];
		int n_layouts;
		int layout;

		int max_videos;
		int first_midinote;
    int midi_port;
		float fade_in;
		float fade_out;
		int sustain_mode;
		int saturation;

		bool blending_multiply;
		bool isDynamic = false;
		bool isFading = true;
		bool dynIsSpeed = false;
        bool layoutShuffle = false;

		float dynDecay = 0.98;
		float dynToSpeed(int movieN);
		void decayDyn();

		ofLoopType loopState = OF_LOOP_NORMAL;
		ofColor videoColor;

		void newMidiMessage(ofxMidiMessage& eventArgs);
		ofxMidiIn midiIn;
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

        void drawVideoInLayout(int movieN);

		void findConfig();
		void scanDataDir();
		void initVideoVariables(int key);
		void loadConfig(string path);
		void loadDefaultConfig();
		/*void loadDataDir();
		void loadFolders();
		void loadRandom();*/

		void loadConfigNew(string path);
		void loadMidiMappings();
		void loadSourceGroup(string path, int layout);
		void loadCaptureGroup(int deviceID, int layout);
		void loadRandomGroup(string path,int size);


		void playVideo(int key, float vel);
		void deactivateVideo(int key);
		void stopVideo(int key);
		void changeAllSpeed(float control);
		float tapToSpeed(float t, int k);

		void stopSustain();
        void startSostenuto();
        void stopSostenuto();
        void startSostenutoFreeze();
        void stopSostenutoFreeze();
        void panic();

		void keyPressed(int key);
		void keyReleased(int key);


	private:

		// for multiply blending:
		// register if layoutPos has been initialized (has already a texture)
		int thisLayoutInit[MAX_LAYOUTPOS];

		int screenW;
		int screenH;
		int n_videos;

		bool random;

		float speed; // global
		float sustain;
        float sostenuto;
        float sostenutoFreeze;

		bool active_videos[MAX_VIDEOS];

		bool reset_videos[MAX_VIDEOS] = {false};

		bool sustained_videos[MAX_VIDEOS];
    bool sostenuto_videos[MAX_VIDEOS];
    bool sostenutoFreeze_videos[MAX_VIDEOS];


		float fo_start[MAX_VIDEOS];

		float tapTempo[MAX_VIDEOS];
		float tapSpeed[MAX_VIDEOS];
		float dyn[MAX_VIDEOS];

		float startPos[MAX_VIDEOS];

		float lastDecayTime;
		// keep track of n_activeVideos for volume settings
		int n_activeVideos;
		float volume;

		string pitchBendFunc="global_speed";
		enum class MidiCommand {
			fade_in,
			fade_out,
			global_speed,
			layout_change,
			saturation,
			sustain,
			sostenuto,
			sostenuto_freeze,
			dynamics_switch,
			fade_switch,
			blending_multiply_switch,
			source_shuffle_switch,
			sustain_mode,
			loop_mode,
            speed_dynamics,
            layout_shuffle
		};

		std::map<string, MidiCommand> midiMappingsStringsToCommand = {
		 {"fade_in", MidiCommand::fade_in},
		 {"fade_out", MidiCommand::fade_out},
		 {"global_speed", MidiCommand::global_speed},
		 {"layout_change", MidiCommand::layout_change},
		 {"saturation" , MidiCommand::saturation},
		 {"sustain", MidiCommand::sustain},
		 {"sostenuto", MidiCommand::sostenuto},
		 {"sostenuto_freeze", MidiCommand::sostenuto_freeze},
		 {"dynamics_switch", MidiCommand::dynamics_switch},
		 {"fade_switch", MidiCommand::fade_switch},
		 {"blending_multiply_switch", MidiCommand::blending_multiply_switch},
		 {"source_shuffle_switch", MidiCommand::source_shuffle_switch},
		 {"sustain_mode", MidiCommand::sustain_mode },
		 {"loop_mode", MidiCommand::loop_mode },
         {"speed_dynamics", MidiCommand::speed_dynamics },
         {"layout_shuffle", MidiCommand::layout_shuffle }

		};

		std::map<string, int> midiMappings = {
		 {"fade_in", 22},
		 {"fade_out", 23},
		 {"global_speed", -1},
		 {"layout_change", 1},
		 {"saturation" , 2},
		 {"sustain", 64},
		 {"sostenuto", 65},
		 {"sostenuto_freeze", 66},
		 {"dynamics_switch", 24},
		 {"fade_switch", 25},
		 {"blending_multiply_switch", 26},
		 {"source_shuffle_switch", 27},
		 {"sustain_mode", 28 },
		 {"loop_mode", 29 },
         {"speed_dynamics", 30 },
         {"layout_shuffle", 31 }

		};


		std::map<int,MidiCommand> midiMapByValue;

		int midiMaxVal;
		// LAYOUTS
		// 1: dual vertical (0-1)
		// 2: dual horizontal (0-1)
		// 3: triple 2+1 (0-2)
        // 4: triptych (0-2)
		// 5: quad (0-3)

		int layoutCount[N_LAYOUTS+1][MAX_LAYOUTPOS]={0};

		int** layoutConf;


};
