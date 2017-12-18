// we need to restructure the data folder
// each folder should contain an "audio" subfolder with all the audios

// then we load midimappings.json


// MAKE DEPENDANTS: (fadeOut?,fade_switch?)

// missing features
// FREEZE stutterStart + harmloop
// RMS
// RANDOM
// AUTOPLAY
// DECAY


(
SynthDef(\vkSampler){|buf,speed=1,t_reset=0,start=0,gate=1,fade_out=0,amp=1,stutterDur=1,stutter=0,t_latchPos=0,stutterPos=0|
	var reset = Select.kr(stutter>0,[0,TDuty.kr(stutterDur,Changed.kr(stutter))]);
	var latchStart = Latch.ar(LocalIn.ar(1),t_latchPos);
	var startPos = Select.kr(stutterPos>0,[start,latchStart]);
	var end = Select.kr(stutter>0,[BufSamples.ir(buf),stutterDur*BufSampleRate.ir(buf)+startPos]);
	var pos = Phasor.ar(t_reset,BufRateScale.kr(buf)*speed,startPos,end,startPos);
	LocalOut.ar(pos);
	Out.ar(0,
		//PlayBuf.ar(2,buf,speed,reset+t_reset,start,1)
		BufRd.ar(2,buf,pos,1)
		//LoopBuf.ar(2,buf,speed,t_reset,startPos,startPos,end)
		*EnvGen.kr(Env.asr(0.05,1,fade_out.clip(0.05,10)),gate,doneAction:2)*amp
	);
}.add
)


(
~vk = ~vk ? ();

~vk.midiCommands = (
    \fade_out: {|val| ~vk.fade_out = val*3},
	\fade_switch:{|val| ~vk.fade_switch = val!=0},
	\global_speed:{|val| ~vk.changeAllSpeed(val) },
	\speed_reverse:{|val| ~vk.speed_reverse = val!=0;~vk.changeAllSpeed(-1) },
	\global_volume:{|val| ~vk.changeAllVolume(val)},


	\sustain:{|val|
		switch(~vk.sustain_mode,
			0,{~vk.setSustain(val)},
			1,{~vk.setSostenuto(val)},
			2,{~vk.setSostenutoFreeze(val)}
		);
	},
	\sostenuto:{|val|~vk.setSostenuto(val)},
	\sostenuto_freeze:{|val|~vk.setSostenutoFreeze(val)},
	\sustain_mode:{|val|~vk.sustain_mode = (val*2).round},
	\sostenuto_freeze_inhibit:{|val| ~vk.sostenutoFreezeInhibit = val!=0},
	\sostenuto_is_freeze:{|val| ~vk.sostenutoIsFreeze = val!=0},



	\rms_normalize:{|val| ~vk.rms_mode = val!=0; ~vk.updateAllVolumes();},
	\rms_correction_pct:{|val| ~vk.rms_correction_pct = val**2;~vk.rms_mode = val!=0; ~vk.updateAllVolumes();},
	\rms_global:{|val| ~vk.rms_global = val!=0; ~vk.updateAllVolumes();},

	//"sound_fade_time",


	\speed_dynamics:{|val| ~vk.dynIsSpeed = val!=0; ~vk.updateAllSpeeds();},
	\dynamics_decay:{|val| ~vk.dynIsDecaying = val!=0},
	\dynamics_volume:{|val| ~vk.dynIsVolume = val!=0},
    \ribattutoSpeed:{|val| ~vk.ribattuto_speed = val!=0; ~vk.updateAllSpeeds();},

	\stutter_mode:{|val| ~vk.stutterDurGlobal = linlin(val,0,1,0.04,7); ~vk.setAllStutter(false,true)},
	\stutter_dur_global:{|val| ~vk.stutterDurGlobal = val/2; ~vk.setAllStutter(false,true)},
	\harmonic_loops:{|val| ~vk.harmonic_loops = val!=0; if(~vk.harmonicLoops){~vk.setAllStutter(false)}},
	\harmonic_loop_base_dur:{|val| ~vk.harmonicLoopBaseDur = val*10; if(~vk.harmonicLoops){~vk.setAllStutter(false,true)}},

	\switch_to_set_0:{|val| ~vk.loadSet(0)},
	\switch_to_set_1:{|val| ~vk.loadSet(1)},
	\switch_to_set_2:{|val| ~vk.loadSet(2)},
	\switch_to_set_3:{|val| ~vk.loadSet(3)},
	\switch_to_set_4:{|val| ~vk.loadSet(4)},
	\switch_to_set_5:{|val| ~vk.loadSet(5)},

);

~vk.init = {|vk,path|

	vk.masterVolume = 1.0;
	vk.masterSpeed = 1.0;
	vk.speed_reverse = false;
	vk.activeSet = 0;
	vk.loadedSets = 0;
	vk.setStart = [0];

	vk.sustain = false;
	vk.sustainedVideos = Order();
	vk.sostenutoVideos = Order();
	vk.sostenutoFreezeVideos = Order();
	vk.ribattuto_speed = false;
	vk.sustain_mode = 0;
	vk.sostenutoFreezeInhibit = false;
	vk.sostenutoIsFreeze = false;

	vk.rms_mode = false;
	vk.rms_correction_pct = 0;
	vk.rms_global = false;

	vk.setAvgRms = Order();
	vk.globalAvgRms = 1;

	vk.fade_out = 0;
	vk.fade_switch = false;

	vk.dynIsVolume = false;
	vk.dynIsDecaying = false;
	vk.dynIsSpeed = false;


	vk.stutterDurGlobal = 1;
	vk.harmonic_loops = false;
	vk.harmonicLoopBaseDur = 1.0;

	vk.samples = Order();
	vk.info = Order(); // speed, volume
	vk.node = Order();
	vk.loadConf(path);

	vk.midiMappings = ();
	vk.firstMidiNote = 0;
	vk.midiPorts = [];
	vk.loadMidiMappings(File.realpath(path+/+".."));
	vk.initMIDI;


};

// loading

~vk.loadFolder =  {|vk,dir|
	var path = PathName(dir+/+"audio");
	var rms  = vk.readRMS(dir+/+"rms");
	"loading folder %\n".postf(dir);
	path.files.do{|file|
		var thisRms = nil;
		var thisBuf = Buffer.read(s,file.fullPath);
		if(rms.notNil){if(rms[file.fullPath.basename.splitext[0]].notNil){rms[file.fullPath.basename.splitext[0]]}};
		vk.samples.add(thisBuf);
		vk.info.add((volume:1.0,speed:1.0,dyn:1.0,active:false,tapTempo:0,tapSpeed:1.0,stutterDur:0.1,rms:thisRms));
		vk.sustainedVideos[vk.samples.size-1]  = false;
		vk.sostenutoVideos[vk.samples.size-1]  = false;
		vk.sostenutoFreezeVideos[vk.samples.size-1]  = false;
		"loaded video n.%\n".postf(vk.samples.size);

	};
};

~vk.loadCapture = {|vk|
	// dummy, not implemented yet
	vk.samples.add(Buffer.alloc(s,44100));
	vk.info.add((volume:1.0,speed:1.0,dyn:1.0,active:false,tapTempo:0,tapSpeed:1.0,stutterDur:0.1));
	"placed capture at %\n".postf(vk.samples.size);

};

~vk.loadMultiple = {|vk,dir|
	var path = PathName(dir);
	path.folders.sort({|a,b| a.fullPath <= b.fullPath}).do{|fold|
		"loading set #%: % \ņ".postf(vk.loadedSets,fold);
		vk.loadConf(fold.fullPath);
		vk.loadedSets = vk.loadedSets+1;
		vk.setStart = vk.setStart.add(vk.samples.size);
		vk.storeSetAvgRms(vk.loadedSets-1);
	};
	vk.storeGlobalAvgRms();
};

~vk.loadConf = {|vk,dir|
	var conf;
	if(File.exists(dir+/+"config.json")){
		conf = JSONFileReader.read(dir+/+"config.json");
		conf.postln;
		conf["sources"].keys.asArray.sort({|a,b| a.asInteger <= b.asInteger}).do{|k|
			"loading source group".postln;
			k.postln;
			switch(conf["sources"][k]["type"],
				"multiple",{vk.loadMultiple(dir)},
				"folder",{vk.loadFolder(dir+/+conf["sources"][k]["src"])},
				"capture",{vk.loadCapture()},
				"random",{/*vk.loadRandom()*//*NOT IMPLEMENTED YET*/},
			);
		}
	}
};

~vk.loadMidiMappings = {|vk,dir|
	var confPath = dir +/+ "midi_mappings.json";
	if(File.exists(confPath).not){confPath = File.realpath(dir+/+"..")+/+"midi_mappings.json";};
	if(File.exists(confPath)){
		var conf;
		"loading midi_mappings".postln;
		conf = JSONFileReader.read(confPath);
		conf.keys.do{|k|
			if(vk.midiCommands.keys.includes(k.asSymbol)){
				vk.midiMappings[conf[k].asInteger] = k.asSymbol ;
			}{
				if(["midi_ports","midi_port","midi_port2"].includesEqual(k)){
					vk.midiPorts = (vk.midiPorts ++ (conf[k].split($,).collect{|str|str.asInteger})).postln;
				}{
					if(k=="first_midinote"){vk.firstMidiNote=conf[k].asInteger;}
				}
			}
		};
	}
};

// RMS

~vk.readRMS = {|vk,path|
	if(File.exists(path)){
		var file = File(path,"r");
		var line = "";
		var rms = ();

		while({line=file.getLine;line.notNil}){
			var name,val;
			#name,val = line.split($ );
			rms[name.splitext[0]] = val;

		};

		rms;
	}{
		nil;
	}
};

~vk.storeSetAvgRms = {|vk,set_n|
	var avgRms = 0;
	(vk.setStart[set_n]..(vk.samples.size-1)).do{|i|
		"adding rms for video #%\n".postf(i);
		avgRms = avgRms + (vk.info[i][\rms] ? 1);
	};

	avgRms = avgRms/(vk.samples.size-vk.setStart[set_n]);
	vk.setAvgRms[set_n] = avgRms;
	"set #% avgRms: %\n".postf(set_n,avgRms);
};

~vk.storeGlobalAvgRms = {|vk|
    var avgRms = 0;
    vk.setAvgRms.do{|val|
		avgRms = avgRms +  (val**2);
    };
    avgRms = avgRms**(-2)/vk.setAvgRms.size;
    vk.globalAvgRms = avgRms;
	"global avgRms: %\n".postf(avgRms);
};

// MIDI

~vk.midiNoteToVideoKey = {|vk,note|

	note = note - vk.firstMidiNote;
	note.postln;
    if(vk.activeSet < (vk.loadedSets-1)){
        vk.setStart[vk.activeSet] + (note%(vk.setStart[vk.activeSet+1]-vk.setStart[vk.activeSet]));
    }{
        vk.setStart[vk.activeSet] + (note%(vk.samples.size-vk.setStart[vk.activeSet]));
    }

};

~vk.initMIDI = {|vk|
	MIDIClient.init(1,1);
	MIDIIn.noteOnZeroAsNoteOff = true;
	vk.midiPorts.asSet.do{|port|
		if(MIDIClient.sources.select{|i|i.device!="System"}[port].notNil){
			MIDIIn.connect(0,MIDIClient.sources.select{|i|i.device!="System"}[port]);
			"Connected to midi port #%: %\n".postf(port,MIDIClient.sources.select{|i|i.device!="System"}[port]);

		}{
			"Couldn't find midi port #%\n".postf(port);
		}
	};
	MIDIdef.noteOn(\vkNoteOn,{|...msg|vk.playSample(vk.midiNoteToVideoKey(msg[1]),msg[0]/127);});
	MIDIdef.noteOff(\vkNoteOff,{|...msg|vk.stopSample(vk.midiNoteToVideoKey(msg[1]));});
	MIDIdef.cc(\vkNoteCC,{|...msg|if(vk.midiMappings[msg[1]]!=nil){vk.midiCommands[vk.midiMappings[msg[1]]].(msg[0]/127)};vk.midiMappings[msg[1]].postln;msg[1].postln});
	MIDIdef.bend(\vkNoteBend,{|...msg|if(vk.midiMappings[-1]!=nil){vk.midiCommands[vk.midiMappings[-1]].(msg[0]/16383)};vk.midiMappings[-1].postln;});


};

// updaters

~vk.updateSampleSpeed = {|vk,key|
	if(vk.info[key]!=nil){
		if(vk.info[key][\active]){
		vk.node[key].set(\speed,
			(vk.masterSpeed*
			if(vk.dynIsSpeed){3**(vk.info[key][\dyn].linlin(0,1,-2,2))}{1}*
					if(vk.ribattuto_speed){vk.info[key][\tapSpeed].postln}{1}).postln
		);
		}
	}
};
~vk.updateAllSpeeds = {|vk|
	vk.samples.indices.do{|k|vk.updateSampleSpeed(k)};
};
~vk.updateSampleVolume = {|vk,key|
	if(vk.info[key]!=nil){
		if(vk.info[key][\active]){
			var correction = 1;
			if(vk.rms_mode){
				correction = vk.setAvgRms[vk.setNumberFromKey(key)]/(vk.info[key][\rms]?1);
				if(vk.rms_global){
					correction = vk.globalAvgRms/(vk.info[key][\rms]?1);
				};
				correction = 1+((correction-1)*vk.rms_correction_pct);
				"#% volume % correction % ".postf(key,vk.masterVolume*if(vk.dynIsVolume){vk.info[key][\dyn]}{1}*correction ,correction);
				"set % avg: %".postf(vk.setNumberFromKey(key),vk.setAvgRms[vk.setNumberFromKey(key)]);
				"".postln;
			};

			vk.node[key].set(\amp,
				(vk.masterVolume*if(vk.dynIsVolume){vk.info[key][\dyn]}{1}*correction).postln
			)
		}
	}
};

~vk.setNumberFromKey ={|vk,key|
    var i = vk.setStart.selectIndices{|start|start>key }[0];
	if(i.notNil){
		(i-1).clip(0,vk.loadedSets)
	}{
		if(vk.loadedSets>0){vk.loadedSets-1}{0};
	}
};


~vk.updateAllVolumes = {|vk|
	vk.samples.indices.do{|k|vk.updateSampleVolume(k)};
};


~vk.setSampleStutter ={|vk,key,current=false,onlyDur=false|
	if(vk.info[key]!=nil){
		var dur = vk.samples[key].duration;

		if(vk.harmonic_loops){
			dur = vk.harmonicLoopDur(key);
		};

		if(vk.sostenutoFreezeVideos[key]?false){
			dur = vk.info[key][\stutterDur]*vk.stutterDurGlobal*vk.samples[key].duration;
		};

		if(vk.info[key][\active] || vk.sostenutoFreezeVideos[key]){

			if(onlyDur){vk.node[key].set(\stutterDur, dur.postln)}{
				var start = if(current){
					// get pos from synth
					vk.node[key].set(\start,0,\stutter,vk.sostenutoFreezeVideos[key] || vk.harmonic_loops,\stutterDur, dur,
						\stutterPos,vk.sostenutoFreezeVideos[key] || vk.harmonic_loops,\t_latchPos,1
					);

				}{
					vk.node[key].set(\start,0,\stutter,(vk.sostenutoFreezeVideos[key] || vk.harmonic_loops).postln,\stutterDur, dur,\stutterPos,0);
					vk.node[key].set(\t_latchPos,1);
					"setting stutter for key %, val: %, dur:% \n".postf(key,(vk.sostenutoFreezeVideos[key] || vk.harmonic_loops),dur);
				};
			};
		};
	};

};
~vk.setAllStutter = {|vk,current=false,onlyDur=false|
	vk.samples.indices.do{|k|vk.setSampleStutter(k,current,onlyDur)};
};

~vk.harmonicLoopDur = {|vk,key|
	var octave = floor((key - vk.setStart[vk.activeSet])/12);
	var ratio = [1,1.0625,1.125,1.2,1.25,1.3333,1.375,1.5,1.625,1.6666,1.75,1.875][(vk.first_midinote + key - vk.setStart[vk.activeSet]) % 12];
	var dur = vk.harmonicLoopBaseDur / (ratio * octave);
	"%  = % / ( % x % )\n".postf(dur,vk.harmonicLoopBaseDur,ratio,octave);
	dur
};
// control functions

~vk.setSustain = {|vk,val|
	vk.sustain = val>0;
	if(vk.sustain.not){vk.stopSustain();};
};
~vk.stopSustain = {|vk|
	vk.info.keysValuesDo{|k,v|vk.info[k][\tapSpeed]=1.0;};
	vk.sustainedVideos.do{|sus,i|
		if(sus){
			vk.stopSample(i);
			vk.sustainedVideos[i] = false;
		}
	}
};
~vk.setSostenuto = {|vk,val|
	if(vk.sostenutoIsFreeze){
		vk.setSostenutoFreeze(val);
	}{
	vk.sostenuto = val>0;
	if(vk.sostenuto){vk.startSostenuto();}{vk.stopSostenuto();};
	}
};
~vk.startSostenuto = {|vk|
	"start sostenuto".postln;
	vk.info.keysValuesDo{|k,v| if(vk.info[k][\active]){vk.sostenutoVideos[k]=true};};
};
~vk.stopSostenuto = {|vk|
	"stop sostenuto".postln;
	vk.sostenutoVideos.keysValuesDo{|k,v| if(v){vk.sostenutoVideos[k]=false;if(vk.info[k][\active].not){vk.stopSample(k)}};};
};
~vk.setSostenutoFreeze = {|vk,val|
	if(vk.sostenutoFreezeInhibit){vk.setSostenuto(val)}{
		vk.sostenutoFreeze = val>0;
		if(vk.sostenutoFreeze){vk.startSostenutoFreeze();}{vk.stopSostenutoFreeze();};
	}
};
~vk.startSostenutoFreeze = {|vk|
	vk.info.keysValuesDo{|k,v| if(vk.info[k][\active]){vk.sostenutoFreezeVideos[k]=true;vk.setSampleStutter(k,true,false);};};
};
~vk.stopSostenutoFreeze = {|vk|
	vk.sostenutoFreezeVideos.keysValuesDo{|k,v| if(v){vk.sostenutoFreezeVideos[k]=false;vk.setSampleStutter(k,true,false);if(vk.info[k][\active].not){vk.stopSample(k)}};};
};

~vk.changeAllSpeed = {|vk,ctrl|
	var scaled = 3**linlin(ctrl,0,1,-2,2);

	if(ctrl>=0){
		vk.masterSpeed = abs(scaled) * if(vk.speed_reverse){-1}{1};
	}{
		// only update speed direction if control < 0
		vk.masterSpeed = abs(vk.masterSpeed) * if(vk.speed_reverse){-1}{1};
	};
	vk.updateAllSpeeds();
};

~vk.changeAllVolume = {|vk,ctrl|
  var scaled =ctrl.linlin(0.0,1.0,-80,0).dbamp;
  if(ctrl==0){scaled=0;};
  vk.masterVolume = scaled;
	vk.masterVolume.postln;
		vk.updateAllVolumes();

};

~vk.tapToSpeed = {|vk,t,k|
  var newSpeed = 1.0;
	if((t>0).and(t<= 10)){
      newSpeed = (linlin(t,0,10,1,0)**10)*20;
  };
  vk.info[k][\tapSpeed] = newSpeed;
};

// TODO: autoplay support
~vk.loadSet={|vk,set_n|

  /*for(auto autoplay : autoplayGroupsForSet[activeSet]){
    cout << "autostop " << endl;
    for(int i=0;i<autoplay[1];i++){
      stopVideo(autoplay[0]+1);
    }
  }*/
	"loading set %\n".postf(set_n%vk.loadedSets);
  vk.activeSet = set_n%vk.loadedSets;

  /*for(auto autoplay : autoplayGroupsForSet[activeSet]){
    cout << "autoplay " << endl;
    for(int i=0;i<autoplay[1];i++){
      playVideo(autoplay[0]+1,1.0);
    }
  }*/
};

// play/stop

~vk.playSample = {|vk,key,vel|

	("playing sample KEY: % of set: % setstart: %\n").postf(key,vk.activeSet,vk.setStart[vk.activeSet]);
	if(key>=0 && key < vk.samples.size){
		vk.info[key][\dyn] = vel;
		if(vk.dynIsVolume){vk.updateSampleVolume(key)};

		if(vk.info[key][\active].not){
			vk.info[key][\active] = true;
			// start playing
			if(vk.node[key].notNil){vk.node[key].free};
			vk.node[key] = Synth(\vkSampler,[\buf,vk.samples[key],\fade_out,if(vk.fade_switch){vk.fadeOut}{0}]);
			vk.updateSampleSpeed(key);
			vk.updateSampleVolume(key);
		}{
			if(vk.sustain && vk.ribattuto_speed){
				// if video is already active and sustain is on (tapping)
				var now = Date.getDate().rawSeconds;
				vk.tapToSpeed(now-vk.info[key][\tapTempo],key);
				vk.info[key][\tapTempo] = now;
				vk.updateSampleSpeed(key);

			}{
				// reset video: TODO
				vk.node[key].set(\t_reset,1);
			}
		};

		vk.sustainedVideos[key] = false;
		//sostenuto_videos[key] = false;
		//sostenutoFreeze_videos[key] = false;

	}
};

~vk.stopSample = {|vk,key|
	"stopping sample %\n".postf(key);

	if(key>=0 && key <  vk.samples.size){

		if(vk.sustain.not && vk.sostenutoVideos[key].not && vk.sostenutoFreezeVideos[key].not){
			// start fadeout
			vk.node[key].set(\fade_out,vk.fadeOut).release;
			vk.info[key][\active] = false;
			"stopped video %\n".postf(key);
		}{
			if(vk.sustain){
				vk.sustainedVideos[key] = true;
			};
			if(vk.sostenutoVideos[key] || vk.sostenutoFreezeVideos[key]){
				vk.info[key][\active] = false;
			}

		}
	}
}

)

~vk.init("/Users/sorenkjaergaard/Desktop/VideoKeyboard/dev/videoKeyboard2/bin/data/");
~vk.samples.select{|sa|sa.numFrames.isNil}.keysValuesDo{|k,v| ~vk.samples[k] = Buffer.read(s,v.path)}

~vk.initMIDI
~vk.samples.indices.do{|sa|~vk.stopSample(sa)}

~vk.samples.collect{|sa|sa.duration}
~vk.samples.array[(0..250)].collect{|sa|sa.duration}

b = Buffer.read(s,~vk.samples[139].path)

b.play

numBuffers
∏
s.cachedBuffersDo{|sa|sa.free}
338 - ~vk.samples.size

~vk.stopSample(39)