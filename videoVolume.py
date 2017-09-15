#!/bin/python

from multiprocessing.dummy import Pool as ThreadPool
import os,sys
import subprocess

def analyzeAudio(path):
    #demux
    if not os.path.isfile(path+".audio.wav"):
        os.system("ffmpeg -i "+path+" -map 0:a "+path+".audio.wav")
    #analyze volume
    proc = subprocess.run(["sox",path+".audio.wav", "-n","stat"], stdout=subprocess.PIPE,stderr=subprocess.STDOUT)

    # extract RMS volume
    #out = float(str(proc.stdout.splitlines()[8]).split(":")[-1].strip().replace("'",""))
    lines = proc.stdout.splitlines();
    if len(lines)>8:
        out = lines[8].split(b":")[-1]
        out = str(float(out))
        # clean_up
        os.system("rm "+path+".audio.wav")
        return((path,out))


allowExt = ".mov"
if len(sys.argv)>2:
    allowExt = sys.argv[2]
    if allowExt[0]!=".":
        allowExt = "."+allowExt

dirs = os.listdir( sys.argv[1] )

dirs = [os.path.join(sys.argv[1],f) for f in dirs if os.path.splitext(f)[1] == allowExt]

if len(dirs) == 0 :
    print("Can't find any file! (ext: "+allowExt+")");
else:
    pool = ThreadPool(4)
    results = pool.map(analyzeAudio, dirs)
    pool.close()
    pool.join()

    wf = open(os.path.join(sys.argv[1],"rms"),"w+")

    for pair in results:
        if pair != None:
            print(" ".join([os.path.basename(pair[0]),pair[1]]),file=wf)
    wf.close()
