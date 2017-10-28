#include "ofxJsonSettings.h"

#define N_LAYOUTS 6 // fullscreen, double v, double h, triple, tryptich, quad


struct SourceGroup {
  short type; // 0: folder, 1:capture, 2:rand, 3:multiple
  short layout;
  short deviceID; // for capture
  int size; // random groups need a size
  bool autoplay;
  string src; // for folder and random groups
  //SourceGroup() : type(0),layout(0),deviceID(0),size(0),src("") {};
};

enum class MidiCommand {
  none,
  fade_in,
  fade_out,
  global_speed,
  layout_change,
  brightness,
        brightness_opacity,
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
  dynamics_decay,
  layout_shuffle,
  global_volume,
  stutter_mode,
        stutter_dur_global,
  dynamics_volume,
  rms_normalize,
  harmonic_loops,
  harmonic_loop_base_dur,
  switch_to_layout_0,
  switch_to_layout_1,
  switch_to_layout_2,
  switch_to_layout_3,
  switch_to_layout_4,
  switch_to_layout_5,
  speed_reverse,
  switch_to_set_0,
  switch_to_set_1,
  switch_to_set_2,
  switch_to_set_3,
  switch_to_set_4,
  switch_to_set_5,
        ribattutoSpeed,
        panic,
        sound_fade_time,
        rms_correction_pct,
        blending_add_switch,
        black_screen
};


class Config {

  public:

      int first_midinote;
      int midi_port;
      int midi_port2;
    std::vector<int> midi_ports;
      float fade_in;
      float fade_out;

      int n_layoutConfs=0; // number of defined layout configurations, for setting a random one

      int n_sources;

      std::vector<SourceGroup> findConfig();
      void loadDefaultConfig();
      std::vector<SourceGroup> loadConfig(string path);
      void loadDefaultMidiMappings();
      void loadMidiMappings();
      void initMidiMappings();

      //std::vector<SourceGroup> sourceGroups;

      std::map<string, MidiCommand> midiMappingsStringsToCommand = {
       {"fade_in", MidiCommand::fade_in},
       {"fade_out", MidiCommand::fade_out},
       {"global_speed", MidiCommand::global_speed},
       {"layout_change", MidiCommand::layout_change},
       {"brightness" , MidiCommand::brightness},
       {"brightness_opacity" , MidiCommand::brightness_opacity},
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
       {"layout_shuffle", MidiCommand::layout_shuffle },
       {"dynamics_decay", MidiCommand::dynamics_decay },
       {"global_volume", MidiCommand::global_volume },
       {"stutter_mode", MidiCommand::stutter_mode },
       {"stutter_dur_global", MidiCommand::stutter_dur_global },
       {"dynamics_volume", MidiCommand::dynamics_volume },
       {"rms_normalize", MidiCommand::rms_normalize },
       {"harmonic_loops", MidiCommand::harmonic_loops },
       {"harmonic_loop_base_dur", MidiCommand::harmonic_loop_base_dur },
       {"switch_to_layout_0", MidiCommand::switch_to_layout_0 },
       {"switch_to_layout_1", MidiCommand::switch_to_layout_1 },
       {"switch_to_layout_2", MidiCommand::switch_to_layout_2 },
       {"switch_to_layout_3", MidiCommand::switch_to_layout_3 },
       {"switch_to_layout_4", MidiCommand::switch_to_layout_4 },
       {"switch_to_layout_5", MidiCommand::switch_to_layout_5 },
       {"speed_reverse",MidiCommand::speed_reverse},
       {"switch_to_set_0", MidiCommand::switch_to_set_0 },
       {"switch_to_set_1", MidiCommand::switch_to_set_1 },
       {"switch_to_set_2", MidiCommand::switch_to_set_2 },
       {"switch_to_set_3", MidiCommand::switch_to_set_3 },
       {"switch_to_set_4", MidiCommand::switch_to_set_4 },
       {"switch_to_set_5", MidiCommand::switch_to_set_5 },
       {"ribattutoSpeed", MidiCommand::ribattutoSpeed },
       {"panic", MidiCommand::panic },
       {"sound_fade_time", MidiCommand::sound_fade_time },
       {"rms_correction_pct", MidiCommand::rms_correction_pct },
       {"blending_add_switch", MidiCommand::blending_add_switch},
       {"black_screen", MidiCommand::black_screen},

      };

    std::map<string, int> midiMappings;
    std::map<int,MidiCommand> midiMapByValue;

      std::vector<std::array<int,(N_LAYOUTS-1)>> layoutConf;

};
