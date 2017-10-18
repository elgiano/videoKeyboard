#include "ofApp.h"

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